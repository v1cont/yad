
function zenity ()
{
    local ARGS
    ARGS=
    for a in "$@"; do
	case $a in
	    --info) ARGS="$ARGS --image=gtk-dialog-info" ;;
	    --question) ARGS="$ARGS --image=gtk-dialog-question" ;;
	    --warning) ARGS="$ARGS --image=gtk-dialog-warning" ;;
	    --error) ARGS="$ARGS --image=gtk-dialog-error" ;;
	    *) ARGS="$ARGS $a"
	esac
    done
    eval set -- $ARGS
    yad $@
}
export -f zenity
