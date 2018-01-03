This container has been compromised with a dropper script "white-carded" onto
the system. The container will run `dropper.sh` which will reach out to
the `dropper_callback` container on port `8080` to a second stage malware and
execute it.

The second stage `loader.sh` will attempt to hide its next stage by replacing (albeit quite poorly)
`/bin/ps` before executing `notevil.sh`.
