#pragma once
#include <QString>

double getProgressPercent(int completed, int total);
QString formatDuration(double seconds);
QString formatDurationHuman(double seconds);
double getDurationFromVLC(const QString& filePath);
