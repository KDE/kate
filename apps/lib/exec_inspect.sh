#!/bin/bash
#
# arguments: <container name> <engine> <cmdline ...>
#   the latter is (e.g.) podman/docker
#
# It then uses <engine> inspect to obtain mounts,
# and uses OOB extradata to pass this along.

function inspect() {
  TEMPLATE='[{{ range .Mounts }}
  [ "{{ .Source }}", "{{ .Destination }}" ],
  {{ end }} [] ]
  '

  container=$1
  # expected to be podman/docker
  engine=$2

  DATA=`$2 inspect --format "$TEMPLATE" "$1"`
  HEADER=$'X-Type: Mounts\r\n\r\n'

  HEADER_S=${#HEADER}
  DATA_S=${#DATA}

  echo -ne  "Content-Length: $(($HEADER_S + $DATA_S))\r\n\r\n"
  echo -n "${HEADER}${DATA}"
}

if [ "x$KATE_EXEC_INSPECT" != "x" ] ; then
  inspect $1 $2
fi

shift
exec "$@"
