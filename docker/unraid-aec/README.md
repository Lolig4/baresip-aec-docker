# Baresip with WebRTC AEC3 for Unraid

Baresip built from this branch, with the `webrtc_aec` module using AEC3 and
all WebRTC APM settings exposed in the baresip config (see
`modules/webrtc_aec/CONFIG.md`).

Build from the repository root:

    docker build -f docker/unraid-aec/Dockerfile -t baresip-aec:aec3 .

Updating to a new Baresip release:

    git fetch origin tag vX.Y.Z
    git rebase vX.Y.Z
