#include "SystemIntegration.h"

#include <QMetaObject>
#include <QMutex>
#include <QMutexLocker>
#include <QPointer>
#include <QStringList>

#include <atomic>
#include <memory>

#import <AppKit/NSWorkspace.h>
#import <Foundation/Foundation.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

namespace {

struct DefaultPlayerRequestState
{
    QPointer<SystemIntegration> owner;
    std::atomic_int remaining = 0;
    QMutex mutex;
    QStringList errors;
};

QString errorDescription(NSError *error)
{
    if (error == nil) {
        return {};
    }
    return QString::fromUtf8(error.localizedDescription.UTF8String);
}

} // namespace

void SystemIntegration::requestMacDefaultPlayer()
{
    NSURL *applicationUrl = NSBundle.mainBundle.bundleURL;
    if (applicationUrl == nil) {
        completeDefaultPlayerRequest(
            false, tr("macOS could not locate the VeyloPlayer application bundle."));
        return;
    }

    NSArray<NSString *> *extensions = @[
        @"mp3", @"m4a", @"aac", @"wav", @"flac", @"ogg",
        @"mp4", @"m4v", @"mov", @"mkv", @"webm", @"avi",
        @"jpg", @"jpeg"
    ];
    NSMutableArray<UTType *> *contentTypes = [NSMutableArray array];
    NSMutableSet<NSString *> *identifiers = [NSMutableSet set];
    for (NSString *extension in extensions) {
        UTType *contentType = [UTType typeWithFilenameExtension:extension];
        if (contentType != nil && ![identifiers containsObject:contentType.identifier]) {
            [identifiers addObject:contentType.identifier];
            [contentTypes addObject:contentType];
        }
    }

    if (contentTypes.count == 0) {
        completeDefaultPlayerRequest(
            false, tr("macOS could not identify VeyloPlayer's supported media types."));
        return;
    }

    auto state = std::make_shared<DefaultPlayerRequestState>();
    state->owner = this;
    state->remaining = static_cast<int>(contentTypes.count);

    NSWorkspace *workspace = NSWorkspace.sharedWorkspace;
    for (UTType *contentType in contentTypes) {
        [workspace setDefaultApplicationAtURL:applicationUrl
                           toOpenContentType:contentType
                            completionHandler:^(NSError *error) {
            if (error != nil) {
                const QMutexLocker locker(&state->mutex);
                state->errors.append(errorDescription(error));
            }

            if (state->remaining.fetch_sub(1) != 1) {
                return;
            }

            SystemIntegration *owner = state->owner.data();
            if (owner == nullptr) {
                return;
            }
            QMetaObject::invokeMethod(owner, [state] {
                if (state->owner == nullptr) {
                    return;
                }

                QStringList errors;
                {
                    const QMutexLocker locker(&state->mutex);
                    errors = state->errors;
                }
                if (errors.isEmpty()) {
                    state->owner->completeDefaultPlayerRequest(
                        true,
                        state->owner->tr("VeyloPlayer is now the default player for its supported media types."));
                } else {
                    state->owner->completeDefaultPlayerRequest(
                        false,
                        state->owner->tr("macOS could not make VeyloPlayer the default for every supported media type: %1")
                            .arg(errors.constFirst()));
                }
            }, Qt::QueuedConnection);
        }];
    }
}
