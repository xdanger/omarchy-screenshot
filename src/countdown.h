// SPDX-FileCopyrightText: 2026 Andy Stewart
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QString>

// Returns only after the countdown has rendered transparent and its surface
// has been removed from the compositor. Capture must start after this call.
bool showCaptureCountdown(int seconds, QString *error);
