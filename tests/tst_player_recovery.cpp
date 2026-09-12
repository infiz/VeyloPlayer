#include "PlayerController.h"

#include <QDataStream>
#include <QFile>
#include <QGuiApplication>
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
