/*
    SPDX-FileCopyrightText: 2026 Artyom Kirnev <kirnevartem30@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

/// Controls error reporting behavior when running queries.
enum class ExecutionMode {
    Interactive, ///< Single query: can show QMessageBox popups for prepare failures
    Batch ///< Multiple queries: suppress popups, errors go to text output only
};
