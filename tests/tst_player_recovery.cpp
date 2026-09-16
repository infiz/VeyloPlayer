#include "PlayerController.h"

#include <QDataStream>
#include <QElapsedTimer>
#include <QFile>
#include <QGuiApplication>
#include <QScreen>
#include <QPixmap>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

namespace {
QByteArray words(std::initializer_list<quint32> values)
{
    QByteArray result;
    QDataStream stream(&result, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    for (const auto value : values) stream << value;
    return result;
}

QByteArray chunk(const char *name, const QByteArray &data)
{
    return QByteArray(name, 4) + words({quint32(data.size())}) + data
        + (data.size() % 2 ? QByteArray(1, '\0') : QByteArray());
}

// Small, self-contained, seekable video: no downloads or external encoders.
QByteArray testVideo()
{
    constexpr quint32 frameSize = 32 * 32 * 3;
    const auto avih = words({100000, frameSize * 10, 0, 0x10, 150, 0, 1,
                            frameSize, 32, 32, 0, 0, 0, 0});
    const auto strh = QByteArray("vidsDIB ", 8)
        + words({0, 0, 0, 1, 10, 0, 150, frameSize, 0xffffffff, 0, 0, 0x00200020});
    const auto strf = words({40, 32, 32, 0x00180001, 0, frameSize, 0, 0, 0, 0});
    const auto header = chunk("LIST", QByteArray("hdrl") + chunk("avih", avih)
        + chunk("LIST", QByteArray("strl") + chunk("strh", strh) + chunk("strf", strf)));
    QByteArray frames("movi");
    QByteArray index;
    for (int frame = 0; frame < 150; ++frame) {
        index += QByteArray("00db") + words({0x10, quint32(frames.size()), frameSize});
        frames += chunk("00db", QByteArray(frameSize, char(frame)));
    }
    return chunk("RIFF", QByteArray("AVI ") + header
        + chunk("LIST", frames) + chunk("idx1", index));
}
}

class PlayerRecoveryTests final : public QObject
{
    Q_OBJECT
private slots:
    void seekingWhilePausedKeepsSelectedPosition();
    void resumesAndPreservesPauseAndStop();
    void localVideoKeepsUp();
};

void PlayerRecoveryTests::seekingWhilePausedKeepsSelectedPosition()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("seek.avi"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    const auto video = testVideo();
    QCOMPARE(file.write(video), video.size());
    file.close();

    PlayerController player;
    player.videoWindow()->resize(160, 160);
    player.videoWindow()->show();
    QVERIFY(player.openFile(path));
    QTRY_VERIFY_WITH_TIMEOUT(player.position() > 500 && player.seekable(), 10000);
    player.pause();
    QTRY_VERIFY(!player.playing());

    // Releasing the slider immediately restores its binding to this value.
    // Exercise forward, backward, and repeated seeks without resuming playback.
    for (const qint64 target : {8000, 3000, 6000}) {
        player.seek(target);
        QCOMPARE(player.position(), target);
        QTest::qWait(400);
        QCOMPARE(player.position(), target);
        QVERIFY(!player.playing());
        QVERIFY(qAbs(libvlc_media_player_get_time(player.mediaPlayer_) - target) < 250);
    }

    player.play();
    QTRY_VERIFY_WITH_TIMEOUT(player.playing() && player.position() > 6000, 5000);
    QVERIFY(player.position() < 8000);
}

void PlayerRecoveryTests::resumesAndPreservesPauseAndStop()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("recovery.avi"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    const auto video = testVideo();
    QCOMPARE(file.write(video), video.size());
    file.close();
    PlayerController player;
    player.videoWindow()->resize(160, 160);
    player.videoWindow()->show();
    QVERIFY(player.openFolder(directory.path()));
    QTRY_VERIFY_WITH_TIMEOUT(player.position() > 500, 10000);
    player.seek(4000);
    QTRY_VERIFY_WITH_TIMEOUT(player.position() >= 4000, 5000);

    // Queue an old end-of-file callback. It must not advance/reset the session
    // once the player has been retired for a graphics reset.
    libvlc_event_t stale{};
    stale.type = libvlc_MediaPlayerEndReached;
    PlayerController::vlcEventCallback(&stale, &player);
    player.graphicsAvailable_ = false;
    player.suspendForGraphicsReset();
    const qint64 saved = player.position();
    QVERIFY(!player.playing());
    QVERIFY(player.loading());
    QVERIFY(!player.mediaPlayer_);
    const QStringList queue = player.recursiveQueue_;
    QVERIFY(!player.openFolder(directory.path()));
    QCOMPARE(player.recursiveQueue_, queue);
    QTRY_VERIFY_WITH_TIMEOUT(!player.retiringPlayerThread_, 10000);
    QVERIFY(!player.ended_);
    QCOMPARE(player.position(), saved);
    player.graphicsAvailable_ = true;
    player.resumeAfterGraphicsReset();
    QTRY_VERIFY_WITH_TIMEOUT(player.playing() && player.position() > saved, 10000);
    QCOMPARE(player.currentFilePath(), path);
    QCOMPARE(player.recursiveQueue_, queue);

    player.pause();
    QTRY_VERIFY(!player.playing());
    player.graphicsAvailable_ = false;
    player.suspendForGraphicsReset();
    QTRY_VERIFY_WITH_TIMEOUT(!player.retiringPlayerThread_, 10000);
    player.graphicsAvailable_ = true;
    player.resumeAfterGraphicsReset();
    QTRY_VERIFY_WITH_TIMEOUT(!player.loading() && !player.playing(), 10000);
    const qint64 paused = player.position();
    QTest::qWait(400);
    QCOMPARE(player.position(), paused);

    player.graphicsAvailable_ = false;
    player.suspendForGraphicsReset();
    player.stop();
    QTRY_VERIFY_WITH_TIMEOUT(!player.retiringPlayerThread_, 10000);
    player.graphicsAvailable_ = true;
    player.resumeAfterGraphicsReset();
    QVERIFY(!player.graphicsRecoveryPending_);
    QVERIFY(!player.mediaPlayer_);
    QVERIFY(!player.playing());
    QVERIFY(!player.loading());
    player.playPause();
    QTRY_VERIFY_WITH_TIMEOUT(player.playing(), 10000);
}

void PlayerRecoveryTests::localVideoKeepsUp()
{
    const QString path = qEnvironmentVariable("VEYLO_PLAYBACK_TEST_FILE");
    if (path.isEmpty()) {
        QSKIP("Set VEYLO_PLAYBACK_TEST_FILE to a local video of at least 7 seconds.");
    }
    struct OutputDiagnostics {
        std::atomic_bool direct3d11{false};
        std::atomic_bool planar422{false};
        std::atomic_bool rgba64{false};
        bool verbose = qEnvironmentVariableIsSet("VEYLO_PLAYBACK_TEST_LOG");
    } diagnostics;
    PlayerController player;
    player.videoWindow()->resize(1280, 720);
    player.videoWindow()->show();
    QVERIFY(player.ensureMediaEngine());
    libvlc_log_set(player.vlcInstance_, [](void *opaque, int, const libvlc_log_t *,
                                          const char *format, va_list arguments) {
        auto &output = *static_cast<OutputDiagnostics *>(opaque);
        const QString message = QString::vasprintf(format, arguments);
        if (message.contains(QStringLiteral("using vout display module \"direct3d11\"")))
            output.direct3d11 = true;
        if (message.contains(QStringLiteral("Using pixel format I422_10")))
            output.planar422 = true;
        if (message.contains(QStringLiteral("Using pixel format RGBA64")))
            output.rgba64 = true;
        if (output.verbose) qInfo().noquote() << message;
    }, &diagnostics);
    QVERIFY(player.openFile(path));
    QTRY_VERIFY_WITH_TIMEOUT(player.position() > 500 && player.seekable(), 10000);
    QVERIFY(player.duration() >= 7000);

    auto media = std::unique_ptr<libvlc_media_t, decltype(&libvlc_media_release)>(
        libvlc_media_player_get_media(player.mediaPlayer_), &libvlc_media_release);
    QVERIFY(media);
    libvlc_media_track_t **tracks = nullptr;
    const unsigned count = libvlc_media_tracks_get(media.get(), &tracks);
    double fps = 0;
    for (unsigned index = 0; index < count; ++index) {
        if (tracks[index]->i_type == libvlc_track_video
            && tracks[index]->video->i_frame_rate_den > 0) {
            fps = double(tracks[index]->video->i_frame_rate_num)
                / tracks[index]->video->i_frame_rate_den;
            break;
        }
    }
    libvlc_media_tracks_release(tracks, count);
    QVERIFY(fps > 0);

    for (const bool fullscreen : {false, true}) {
        if (fullscreen) {
            player.videoWindow()->showFullScreen();
            diagnostics.direct3d11 = false;
            diagnostics.planar422 = false;
            diagnostics.rgba64 = false;
            QVERIFY(player.openFile(path));
            media.reset(libvlc_media_player_get_media(player.mediaPlayer_));
            QVERIFY(media);
        }
        // Measure uninterrupted playback after startup. Exercise seeking
        // separately so preroll does not skew the sustained delivery counters.
        QTRY_VERIFY_WITH_TIMEOUT(player.playing() && player.position() >= 2500, 10000);
        QTest::qWait(500);
        libvlc_media_stats_t before{}, after{};
        QVERIFY(libvlc_media_get_stats(media.get(), &before));
        QElapsedTimer elapsed;
        elapsed.start();
        QTest::qWait(3000);
        const double seconds = elapsed.elapsed() / 1000.0;
        QVERIFY(libvlc_media_get_stats(media.get(), &after));
        const int displayed = after.i_displayed_pictures - before.i_displayed_pictures;
        const int lost = after.i_lost_pictures - before.i_lost_pictures;
        qInfo() << path << (fullscreen ? "fullscreen" : "windowed")
                << "displayed" << displayed << "lost" << lost << "seconds" << seconds;
        // LibVLC publishes counters periodically; allow sampling jitter, but
        // reject the sustained ~17 fps / heavy loss of the RGBA64 path.
        QVERIFY(displayed >= fps * seconds * 0.8);
        QVERIFY(lost <= (displayed + lost) * 0.02);
        QVERIFY(player.errorMessage().isEmpty());
        QVERIFY(diagnostics.direct3d11.load());
        if (qEnvironmentVariableIsSet("VEYLO_PLAYBACK_TEST_EXPECT_PLANAR422")) {
            QVERIFY(diagnostics.planar422.load());
            QVERIFY(!diagnostics.rgba64.load());
        }
        player.pause();
        QTRY_VERIFY(!player.playing());
        const QString snapshot = qEnvironmentVariable("VEYLO_PLAYBACK_TEST_SCREENSHOT");
        if (!fullscreen && !snapshot.isEmpty()) {
            // Capture the composited screen: grabbing the native HWND directly
            // returns black for a Direct3D swap chain.
            const QPoint origin = player.videoWindow()->mapToGlobal(QPoint(0, 0));
            const auto capture = player.videoWindow()->screen()->grabWindow(
                0, origin.x(), origin.y(), player.videoWindow()->width(),
                player.videoWindow()->height());
            QVERIFY(!capture.isNull());
            QVERIFY(capture.save(snapshot));
        }
        player.seek(1000);
        player.play();
        QTRY_VERIFY_WITH_TIMEOUT(player.playing() && player.position() >= 1500, 10000);
        player.stop();
    }
    player.videoWindow()->showNormal();
    player.stop();
}

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("VeyloPlayerTests"));
    QCoreApplication::setApplicationName(QStringLiteral("GraphicsRecovery"));
    QTemporaryDir settings;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
    PlayerRecoveryTests tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "tst_player_recovery.moc"
