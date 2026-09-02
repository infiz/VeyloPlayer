#include "ChapterNavigation.h"
#include "ChapterNavigation.h"
#include "FolderSequence.h"
#include "MediaFile.h"
#include "NaturalSort.h"
#include "PlaybackResume.h"
#include "TrackPreference.h"
#include "WindowPlacement.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include <algorithm>

class CoreTests final : public QObject
{
    Q_OBJECT

private slots:
    void naturalNumbersSortNumerically();
    void naturalSortIsCaseInsensitiveAndDeterministic();
    void classifiesSupportedMedia();
    void folderSequenceFiltersAndSortsMedia();
    void recursiveFolderSequenceUsesNaturalRelativePathOrder();
    void imageSequenceDoesNotWrap();
    void resumePositionRejectsTrivialAndNearlyFinishedPlayback();
    void trackPreferencesMatchByLabelThenOrdinal();
    void folderTrackPreferenceIdsAreFolderScoped();
    void chapterNavigationStopsAtMediaBoundaries();
    void timelineChapterPositionsAreVisibleAndOrdered();
    void windowPlacementStaysVisibleOnAvailableScreens();
};

void CoreTests::naturalNumbersSortNumerically()
{
    QStringList values = {
        QStringLiteral("12.mp4"), QStringLiteral("2.mp4"), QStringLiteral("1.mp4"),
        QStringLiteral("11.mp4"), QStringLiteral("10.mp4"), QStringLiteral("3.mp4"),
    };
    std::sort(values.begin(), values.end(), veylo::NaturalLess());
    QCOMPARE(values, QStringList({
        QStringLiteral("1.mp4"), QStringLiteral("2.mp4"), QStringLiteral("3.mp4"),
        QStringLiteral("10.mp4"), QStringLiteral("11.mp4"), QStringLiteral("12.mp4"),
    }));
}

void CoreTests::naturalSortIsCaseInsensitiveAndDeterministic()
{
    QVERIFY(veylo::naturalCompare(QStringLiteral("clip2.MP4"), QStringLiteral("clip10.mp4")) < 0);
    QVERIFY(veylo::naturalCompare(QStringLiteral("clip1.mp4"), QStringLiteral("clip01.mp4")) < 0);
    QVERIFY(veylo::naturalCompare(QStringLiteral("A.mp4"), QStringLiteral("a.mp4")) != 0);
}

void CoreTests::classifiesSupportedMedia()
{
    QCOMPARE(veylo::mediaKindForPath(QStringLiteral("song.FLAC")), veylo::MediaKind::Audio);
    QCOMPARE(veylo::mediaKindForPath(QStringLiteral("movie.MkV")), veylo::MediaKind::Video);
    QCOMPARE(veylo::mediaKindForPath(QStringLiteral("photo.JPEG")), veylo::MediaKind::Image);
    QCOMPARE(veylo::mediaKindForPath(QStringLiteral("notes.txt")), veylo::MediaKind::None);
}

void CoreTests::folderSequenceFiltersAndSortsMedia()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const auto touch = [&temporaryDirectory](const QString &name) {
        QFile file(temporaryDirectory.filePath(name));
        QVERIFY(file.open(QIODevice::WriteOnly));
    };
    touch(QStringLiteral("1.mp4"));
    touch(QStringLiteral("2.mp3"));
    touch(QStringLiteral("10.mkv"));
    touch(QStringLiteral("3.jpg"));
    touch(QStringLiteral("4.txt"));

    const QString current = temporaryDirectory.filePath(QStringLiteral("1.mp4"));
    QCOMPARE(QFileInfo(veylo::FolderSequence::adjacentFile(
        current, veylo::MediaKind::Video, 1)).fileName(), QStringLiteral("2.mp3"));
    QCOMPARE(QFileInfo(veylo::FolderSequence::adjacentFile(
        temporaryDirectory.filePath(QStringLiteral("2.mp3")),
        veylo::MediaKind::Audio, 1)).fileName(), QStringLiteral("10.mkv"));
}

void CoreTests::recursiveFolderSequenceUsesNaturalRelativePathOrder()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QDir root(temporaryDirectory.path());
    const auto touch = [&root](const QString &relativePath) {
        const QFileInfo info(root.filePath(relativePath));
        QVERIFY(QDir().mkpath(info.absolutePath()));
        QFile file(info.absoluteFilePath());
        QVERIFY(file.open(QIODevice::WriteOnly));
    };

    touch(QStringLiteral("1.mp4"));
    touch(QStringLiteral("Season2/episode2.mp3"));
    touch(QStringLiteral("Season2/episode10.mkv"));
    touch(QStringLiteral("Season10/episode1.mp4"));
    touch(QStringLiteral("Season2/poster.jpg"));
    touch(QStringLiteral("Season2/notes.txt"));

    const auto relativePaths = [&root](const QStringList &paths) {
        QStringList result;
        for (const QString &path : paths) {
            result.append(QDir::fromNativeSeparators(root.relativeFilePath(path)));
        }
        return result;
    };

    QCOMPARE(relativePaths(veylo::FolderSequence::recursiveFiles(
                 temporaryDirectory.path(), veylo::MediaKind::Video)),
             QStringList({
                 QStringLiteral("1.mp4"),
                 QStringLiteral("Season2/episode2.mp3"),
                 QStringLiteral("Season2/episode10.mkv"),
                 QStringLiteral("Season10/episode1.mp4"),
             }));
    QCOMPARE(relativePaths(veylo::FolderSequence::recursiveFiles(
                 temporaryDirectory.path(), veylo::MediaKind::Image)),
             QStringList({QStringLiteral("Season2/poster.jpg")}));
}

void CoreTests::imageSequenceDoesNotWrap()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    for (const QString &name : {QStringLiteral("photo1.jpg"),
                                QStringLiteral("photo2.jpeg"),
                                QStringLiteral("photo10.JPG")}) {
        QFile file(temporaryDirectory.filePath(name));
        QVERIFY(file.open(QIODevice::WriteOnly));
    }

    const QString first = temporaryDirectory.filePath(QStringLiteral("photo1.jpg"));
    const QString second = veylo::FolderSequence::adjacentFile(first, veylo::MediaKind::Image, 1);
    const QString last = veylo::FolderSequence::adjacentFile(second, veylo::MediaKind::Image, 1);
    QCOMPARE(QFileInfo(second).fileName(), QStringLiteral("photo2.jpeg"));
    QCOMPARE(QFileInfo(last).fileName(), QStringLiteral("photo10.JPG"));
    QVERIFY(veylo::FolderSequence::adjacentFile(first, veylo::MediaKind::Image, -1).isEmpty());
    QVERIFY(veylo::FolderSequence::adjacentFile(last, veylo::MediaKind::Image, 1).isEmpty());
}

void CoreTests::resumePositionRejectsTrivialAndNearlyFinishedPlayback()
{
    QCOMPARE(veylo::normalizedResumePosition(4'999, 120'000), 0);
    QCOMPARE(veylo::normalizedResumePosition(30'000, 120'000), 30'000);
    QCOMPARE(veylo::normalizedResumePosition(111'000, 120'000), 0);
    QCOMPARE(veylo::normalizedResumePosition(-100, 0), 0);
}

void CoreTests::trackPreferencesMatchByLabelThenOrdinal()
{
    const QList<veylo::TrackOption> originalTracks = {
        {QStringLiteral("English"), 101, false},
        {QStringLiteral("Spanish"), 205, false},
    };
    const veylo::SavedTrackPreference spanish{
        QStringLiteral("spanish"), 1, true};
    QCOMPARE(veylo::bestTrackId(originalTracks, spanish, 101), 205);

    const QList<veylo::TrackOption> renamedTracks = {
        {QStringLiteral("Track A"), 7, false},
        {QStringLiteral("Track B"), 9, false},
        {QStringLiteral("Spanish — External"), 12, true},
    };
    QCOMPARE(veylo::bestTrackId(renamedTracks, spanish, 7), 9);

    const QList<veylo::TrackOption> subtitleTracks = {
        {QStringLiteral("Off"), -1, false},
        {QStringLiteral("English"), 4, false},
    };
    const veylo::SavedTrackPreference subtitlesOff{
        QStringLiteral("Off"), 0, false};
    QCOMPARE(veylo::bestTrackId(subtitleTracks, subtitlesOff, 4), -1);
}

void CoreTests::folderTrackPreferenceIdsAreFolderScoped()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString first = temporaryDirectory.filePath(QStringLiteral("episode1.mkv"));
    const QString second = temporaryDirectory.filePath(QStringLiteral("episode2.mkv"));
    QCOMPARE(veylo::folderPreferenceIdForMediaPath(first),
             veylo::folderPreferenceIdForMediaPath(second));

    const QString nestedFolder = temporaryDirectory.filePath(QStringLiteral("Season2"));
    QVERIFY(QDir().mkpath(nestedFolder));
    const QString nested = QDir(nestedFolder).filePath(QStringLiteral("episode1.mkv"));
    QVERIFY(veylo::folderPreferenceIdForMediaPath(first)
            != veylo::folderPreferenceIdForMediaPath(nested));
}

void CoreTests::chapterNavigationStopsAtMediaBoundaries()
{
    QCOMPARE(veylo::adjacentChapterIndex(0, 3, 1), std::optional<int>(1));
    QCOMPARE(veylo::adjacentChapterIndex(1, 3, 1), std::optional<int>(2));
    QCOMPARE(veylo::adjacentChapterIndex(1, 3, -1), std::optional<int>(0));
    QVERIFY(!veylo::adjacentChapterIndex(2, 3, 1).has_value());
    QVERIFY(!veylo::adjacentChapterIndex(0, 3, -1).has_value());
    QVERIFY(!veylo::adjacentChapterIndex(0, 1, 1).has_value());
}

void CoreTests::timelineChapterPositionsAreVisibleAndOrdered()
{
    QCOMPARE(veylo::timelineChapterPositions(
                 {30'000, 0, 10'000, 30'000, 60'000, -1}, 60'000),
             QList<qint64>({10'000, 30'000}));
    QCOMPARE(veylo::timelineChapterPositions({5'000, 10'000}, 0),
             QList<qint64>({5'000, 10'000}));
}

void CoreTests::windowPlacementStaysVisibleOnAvailableScreens()
{
    const QList<QRect> screens = {
        QRect(0, 0, 1920, 1040),
        QRect(1920, 0, 2560, 1400),
    };

    QCOMPARE(veylo::visibleWindowGeometry(QRect(2200, 120, 1120, 720), screens,
                                          QSize(720, 480)),
             QRect(2200, 120, 1120, 720));
    QCOMPARE(veylo::visibleWindowGeometry(QRect(5000, 200, 1120, 720), screens,
                                          QSize(720, 480)),
             QRect(400, 160, 1120, 720));
    QCOMPARE(veylo::visibleWindowGeometry(QRect(-100, -100, 4000, 2000),
                                          {screens.constFirst()},
                                          QSize(720, 480)),
             QRect(0, 0, 1920, 1040));
}

QTEST_MAIN(CoreTests)
#include "tst_core.moc"
