#include "timeutils.h"
#include <vlc/vlc.h>
#include <QFileInfo>
#include <cmath>

double getProgressPercent(int completed, int total) {
    if (total <= 0) return 0.0;
    return (double)completed / total * 100.0;
}

QString formatDuration(double seconds) {
    if (seconds <= 0) return "0:00";
    int total = (int)std::round(seconds);
    int h = total / 3600;
    int m = (total % 3600) / 60;
    int s = total % 60;
    if (h > 0)
        return QString("%1:%2:%3").arg(h).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));
    return QString("%1:%2").arg(m).arg(s, 2, 10, QChar('0'));
}

QString formatDurationHuman(double seconds) {
    int total = (int)std::round(seconds);
    int weeks = total / (3600 * 24 * 7);
    int days = (total % (3600 * 24 * 7)) / (3600 * 24);
    int hours = (total % (3600 * 24)) / 3600;
    int mins = (total % 3600) / 60;
    QStringList parts;
    if (weeks > 0) parts << QString::number(weeks) + "w";
    if (days > 0) parts << QString::number(days) + "d";
    if (hours > 0) parts << QString::number(hours) + "h";
    if (mins > 0) parts << QString::number(mins) + "m";
    if (parts.isEmpty()) return "<1m";
    return parts.join(" ");
}

double getDurationFromVLC(const QString& filePath) {
    if (!QFileInfo::exists(filePath)) return 0.0;
    libvlc_instance_t* inst = libvlc_new(0, nullptr);
    if (!inst) return 0.0;
    libvlc_media_t* media = libvlc_media_new_path(inst, filePath.toUtf8().constData());
    if (!media) { libvlc_release(inst); return 0.0; }
    libvlc_media_parse_with_options(media, libvlc_media_parse_local, 3000);
    double dur = libvlc_media_get_duration(media) / 1000.0;
    libvlc_media_release(media);
    libvlc_release(inst);
    return dur < 0 ? 0.0 : dur;
}
