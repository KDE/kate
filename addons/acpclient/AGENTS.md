/*
    SPDX-FileCopyrightText: 2026 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

# ACP Client Plugin for Kate - Developer Guide

This document provides an overview of the Agent Client Protocol (ACP) client plugin for Kate, its architecture, and guidelines for development.

**Note:** Some parts of this plugin were adapted from the LSP Client plugin.

## Overview

The ACP Client plugin enables Kate to communicate with ACP-compliant agent servers (like Mistral Vibe, Claude Code, etc.) using the Agent Client Protocol. It provides:

- Interactive chat interface with ACP agents
- Session management (create, load, resume, close)
- Tool call support with permission management
- Progress and usage tracking
- Server configuration and management

## Architecture

### Component Hierarchy

```
ACPClientPlugin (KTextEditor::Plugin)
├── ACPClientPluginView (KXMLGUIClient)
│   ├── ACPClientChatWidget (QWidget)
│   │   └── ACPChatMessageWidget (QWidget)
│   └── Actions (menu/toolbar items)
│
└── ACPClientServerManager (QObject)
    └── ACPClientServer (QObject)
        └── QProcess (server connection)
```

### Core Files

| File | Purpose |
|------|---------|
| `acpclientplugin.h/.cpp` | Main plugin class, configuration management |
| `acpclientpluginview.h/.cpp` | View integration, actions, tool view management |
| `acpclientservermanager.h/.cpp` | Manages multiple ACP server connections |
| `acpclientserver.h/.cpp` | Individual server connection (QProcess-based) |
| `acpclientprotocol.h/.cpp` | ACP protocol v1 implementation |
| `acpclientchatwidget.h/.cpp` | Chat UI widget with message display |
| `acpchatmessagewidget.h/.cpp` | Individual message widget with rich formatting |
| `acpclientconfigpage.h/.cpp` | Configuration page for Kate settings |
| `acpserverdialog.h/.cpp` | Dialog for adding/editing server configurations |

### UI Files

| File | Purpose |
|------|---------|
| `acpclientchat.ui` | Chat widget UI (input, display, buttons) |
| `acpconfigwidget.ui` | Configuration widget UI |
| `acpserverdialog.ui` | Server configuration dialog UI |
| `ui.rc` | KPart GUI XML (menus, toolbars) |
| `plugin.qrc` | Qt resource file |
| `settings.json` | Default server configurations |

## Data Flow

### Message Flow: User → Agent

```
User types message
    ↓
ACPClientChatWidget::sendMessage()
    ↓
ACPClientPluginView::sessionRequested() signal
    ↓
ACPClientPlugin::createView() connects to:
    ACPClientServerManager::createSession()
    ↓
    ACPClientServer::sendMessage() [session/new]
    ↓
    Server responds with sessionId
    ↓
    ACPClientServerManager::sendPrompt(sessionId, message)
    ↓
    ACPClientServer::sendMessage() [session/prompt]
```

### Message Flow: Agent → User

```
ACPClientServer receives JSON-RPC message
    ↓
ACPClientServer::messageReceived() signal
    ↓
ACPClientServerManager::onServerMessageReceived()
    ↓
Handles various message types:
    - session/update → ACPClientChatWidget::onServerMessageReceived()
    - session/request_permission → ACPClientChatWidget::onPermissionRequested()
    - progress → ACPClientChatWidget::onServerMessageReceived()
    ↓
ACPClientChatWidget displays message in UI
```

## ACP Protocol Implementation

The plugin implements ACP Protocol v1 (JSON-RPC 2.0 based) as defined in `acpclientprotocol.h`.

### Supported Methods (Client → Server)

- `initialize` - Initialize the connection
- `session/new` - Create a new session
- `session/load` - Load an existing session
- `session/resume` - Resume a session
- `session/close` - Close a session
- `session/prompt` - Send a prompt to a session
- `session/cancel` - Cancel current operation
- `session/request_permission` - Request permission for tool calls
- `tools/list` - List available tools
- `tools/call` - Call a tool

### Supported Notifications (Server → Client)

- `session/update` - Session state updates (agent messages, tool calls, etc.)
- `$/progress` - Progress notifications
- `$/cancel_request` - Cancellation requests

### Session Update Types

Defined in `acpclientprotocol.h`:

- `SESSION_UPDATE_AGENT_MESSAGE_CHUNK` - Streaming agent message
- `SESSION_UPDATE_PLAN` - Agent plan updates
- `SESSION_UPDATE_TOOL_CALL` - Tool call notification
- `SESSION_UPDATE_TOOL_CALL_UPDATE` - Tool call progress
- `SESSION_UPDATE_USAGE_UPDATE` - Token/cost usage
- `SESSION_UPDATE_MODE` - Mode changes
- `SESSION_UPDATE_AVAILABLE_COMMANDS` - Available commands

## Message Widget Types

`ACPChatMessageWidget` supports these message types (see `MessageType` enum):

- **User** - User's messages (blue styling)
- **Agent** - Agent's text responses (green styling)
- **System** - System messages (gray styling)
- **Plan** - Plan/step updates (blue border, structured display)
- **ToolCall** - Tool call notifications (orange styling)
- **ToolCallUpdate** - Tool call progress (blue styling)
- **Usage** - Usage/token information (purple styling)
- **PermissionRequest** - Inline permission prompts with Allow/Reject buttons

## Configuration

### Plugin Options

Stored in KConfig (`acpclient` group):

- `AutoStartSession` - Start session automatically on Kate startup
- `ShowNotifications` - Show ACP notifications in Kate
- `ShowToolCalls` - Display tool call details in chat
- `ShowProgress` - Display progress updates
- `DebugMode` - Enable debug logging
- `ToolCallPermission` - Permission mode: AllowAll, DenyAll, AskEachTime
- `AllowedServerCommandLines` - Whitelist of allowed server commands
- `BlockedServerCommandLines` - Blacklist of blocked server commands

### Server Configuration

Stored in `~/.config/kate/acpclient/settings.json`:

```json
{
  "version": "1.0",
  "acp_protocol_version": "2.0",
  "servers": [
    {
      "name": "Mistral Vibe (vibe-acp)",
      "version": "1.0",
      "command": "vibe-acp",
      "arguments": [],
      "auto_start": true,
      "metadata": {...}
    }
  ],
  "options": {...}
}
```

## Development Guidelines

### Adding New Features

1. **New Message Types**: Extend `ACPChatMessageWidget::MessageType` and add handling in `updateContentDisplay()`

2. **New Protocol Methods**: Add constants to `acpclientprotocol.h` and implement in `acpclientprotocol.cpp`

3. **New Actions**: Add to `ACPClientPluginView::setupActions()` and `ui.rc`

4. **New Config Options**: Add to `ACPClientPluginOptions` struct and update `readConfig()`/`writeConfig()`

### Known Issues & TODOs

| Location | Issue | Priority |
|----------|-------|----------|
| `acpclientpluginview.cpp:151` | Implement session manager dialog | Medium |
| `acpclientpluginview.cpp:157` | Implement server configuration dialog | Medium |
| `acpclientpluginview.cpp:162` | Implement tool palette | Low |
| `acpclientplugin.cpp:106` | `message->show()` call is commented out | Low |

### Debugging

Enable debug output:

```bash
# Enable all ACP client debug messages
ACPCLIENT_DEBUG=1 kate

# Or via environment variable
export ACPCLIENT_DEBUG=1
```

Debug output uses the `kateacpclientplugin` logging category.

## Testing

The plugin compiles as part of Kate's build system. To test:

```bash
cd /data/home/sandbox/projects/kde/build/kate
cmake --build . -j4
kate
```

Then enable the plugin via:
- Settings → Configure Kate → Plugins → ACP Client
- Or via Kate's plugin menu

### Testing with vibe-acp

1. Ensure `vibe-acp` is installed and in PATH
2. Start Kate
3. Open ACP Chat tool view (View → Tool Views → ACP Chat)
4. Click "New Session" button
5. Type messages in the input field

## File Structure

```
addons/acpclient/
├── CMakeLists.txt           # Build configuration
├── AGENTS.md               # This file
├── plugin.qrc              # Qt resources
├── settings.json           # Default server configs
├── ui.rc                   # KPart GUI XML
│
├── Headers (.h):
│   ├── acpclientplugin.h
│   ├── acpclientpluginview.h
│   ├── acpclientservermanager.h
│   ├── acpclientserver.h
│   ├── acpclientprotocol.h
│   ├── acpclientchatwidget.h
│   ├── acpchatmessagewidget.h
│   ├── acpclientconfigpage.h
│   ├── acpserverdialog.h
│   └── acpclient_debug.h
│
├── Source (.cpp):
│   ├── acpclientplugin.cpp
│   ├── acpclientpluginview.cpp
│   ├── acpclientservermanager.cpp
│   ├── acpclientserver.cpp
│   ├── acpclientprotocol.cpp
│   ├── acpclientchatwidget.cpp
│   ├── acpchatmessagewidget.cpp
│   ├── acpclientconfigpage.cpp
│   └── acpserverdialog.cpp
│
└── UI Files (.ui):
    ├── acpclientchat.ui
    ├── acpconfigwidget.ui
    └── acpserverdialog.ui
```

## Key Classes

### ACPClientPlugin

Main plugin class that:
- Manages plugin lifecycle
- Stores configuration
- Creates views for each Kate main window
- Provides access to server manager
- Handles command line permission requests

### ACPClientPluginView

View class that:
- Integrates with Kate's GUI (menus, toolbars)
- Creates and manages the chat tool view
- Handles action triggers
- Manages connections between UI and server manager

### ACPClientServerManager

Singleton that:
- Manages multiple server connections
- Handles session lifecycle
- Routes messages between servers and UI
- Manages permission requests

### ACPClientServer

Individual server connection that:
- Manages QProcess for server I/O
- Handles JSON-RPC message parsing
- Tracks request/response pairs
- Emits signals for server events

### ACPClientChatWidget

Chat UI widget that:
- Displays conversation history
- Handles user input
- Shows agent responses, tool calls, plans
- Manages permission request UI
- Tracks session state

### ACPChatMessageWidget

Individual message widget that:
- Displays formatted messages with timestamps
- Handles different message types with appropriate styling
- Supports inline permission requests with buttons

## Protocol Notes

- The plugin uses JSON-RPC 2.0 with ACP-specific methods
- Protocol version is negotiated during `initialize`
- Messages are newline-delimited JSON over stdin/stdout
- Each message is parsed and routed based on its type

## Useful Patterns

### Sending a Request

```cpp
ACP::SomeParams params;
// ... set params ...
qint64 requestId = ACP::ACPProtocol::generateRequestId();
QJsonDocument request = ACP::ACPProtocol::createSomeRequest(params, requestId);
server->sendMessage(request);
```

### Handling a Response

```cpp
// In server or server manager
connect(server, &ACPClientServer::messageReceived, this, [this](const QJsonDocument &doc) {
    ACP::ACPMessage message;
    if (ACP::ACPProtocol::parseMessage(doc, message)) {
        if (message.isResponse) {
            // Handle response
        } else if (message.isNotification) {
            // Handle notification
        }
    }
});
```

### Adding a New Message Type

```cpp
// 1. Add to MessageType enum in acpchatmessagewidget.h
// 2. Add case to updateContentDisplay() in acpchatmessagewidget.cpp
// 3. Add styling in getTypeStyle()
// 4. Add label in getTypeLabel()
```

## Resources

- ACP Protocol Specification: https://agentclientprotocol.com
- Mistral Vibe: https://github.com/mistralai/vibe
- Kate Plugin Development: https://docs.kde.org/stable5/en/katepart/katepart/dev-plugins.html
