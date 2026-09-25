# Kate Deployment Plugin Plan

## Goal

Add a project-aware deployment plugin that keeps development files local while
allowing explicit uploads to an SSH server through SFTP. The plugin also
provides a Remote Host sidebar for browsing and directly editing deployed
files.

## Initial Scope

- Configure one SFTP target per Kate project.
- Store non-secret settings in `.kateproject` or `.kateproject.local`.
- Upload the current file with a configurable shortcut.
- Upload all files known to the active Kate project.
- Create missing remote directories.
- Upload through a temporary remote file and rename it into place.
- Apply project-relative exclusion patterns.
- Show transfer progress and errors in a File Transfer toolview.
- Browse the configured remote root in a Remote Host toolview.
- Open remote files directly as `sftp://` documents.
- Open or upload the mapped local file from the remote context menu.

Automatic upload on save, download/synchronization, conflict detection,
automatic remote deletion, and server groups are intentionally deferred.

## Project Configuration

```json
{
    "deployment": {
        "host": "example.org",
        "port": 22,
        "user": "deploy",
        "localRoot": ".",
        "remoteRoot": "/var/www/example",
        "exclude": [
            ".git",
            ".kateproject*",
            "build"
        ]
    }
}
```

`localRoot` is relative to the project base directory. `remoteRoot` must be an
absolute path. Passwords and private keys are never persisted by the plugin;
authentication, host-key verification, and wallet integration are delegated to
KIO.

The configuration dialog writes a complete personal override to
`.kateproject.local`, preserving unrelated keys. Teams can move suitable
non-secret values into `.kateproject` when they should be shared.
The dialog requires a file-backed `.kateproject`; generated in-memory projects
can consume an existing deployment map but cannot persist one in this version.

## Architecture

### Configuration and Mapping

The mapping layer parses the project map, validates local and remote roots, and
performs component-aware forward and reverse mapping. Existing local paths are
canonicalized so that symlinks cannot escape the configured local root. Remote
URLs are assembled with `QUrl`, never string concatenation.

### Transfer Engine

Transfers use asynchronous KIO jobs:

1. `KIO::mkpath` creates the destination parent hierarchy.
2. `KIO::file_copy` uploads to a unique temporary name in that directory.
3. `KIO::rename` replaces the final destination.
4. Failures and cancellation trigger best-effort temporary-file cleanup.

Project uploads use a serial queue. Serial operation avoids stale transfers
overtaking newer ones and keeps authentication/error handling predictable.

### Remote Host Toolview

The left-side Remote Host toolview uses `KUrlNavigator` and `KDirOperator`,
restricted to SFTP. Navigation and actions are validated against the configured
remote root rather than relying on the UI restriction as a security boundary.
This is lexical URL confinement; remote symlinks are interpreted by the server
and must not be treated as a server-side security boundary.

Double-clicking a remote file opens it directly in Kate. The context menu also
offers **Open Mapped Local File** and **Upload Mapped Local File** when a valid
local counterpart exists.

### File Transfer Toolview

The bottom toolview records queued, running, successful, failed, and cancelled
operations. It provides cancel and clear actions and refreshes the remote view
after successful uploads.

## Verification

- Unit-test configuration parsing and validation.
- Test URL encoding, IPv6 hosts, forward/reverse mapping, traversal rejection,
  symlink escapes, and exclusions.
- Build the plugin with the repository's supported Qt and KF6 versions.
- Exercise uploads with KIO's test worker where possible; a live SSH service is
  not required for the unit-test suite.

## Follow-up Work

- Compare local and remote files in Kate's diff UI.
- Download selected remote files to their mapped local paths.
- Preview and apply bidirectional folder synchronization.
- Detect remote modifications before overwrite.
- Add optional upload-on-save behavior with save coalescing.
- Support reusable profiles and deployment groups.
