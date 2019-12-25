function check_root {
    if [[ $EUID -ne 0 ]]; then
		echo -e "\e[31mThis script must be run as root\e[0m"
		exit 1
    fi
}


function automount_disable {
    GSETTINGS=$(which gsettings || true)
    echo "gsettings is $GSETTINGS"
    if [ ! -z "$GSETTINGS" ]
    then
	pp_step "disabling gtk automount"
	gsettings set org.gnome.desktop.media-handling automount false 2>/dev/null
	gsettings set org.gnome.desktop.media-handling automount-open false 2>/dev/null
    fi
}


COMMON_UTILS_CHECK_DIR=
export COMMON_UTILS_CHECK_DIR

function check_dir {
	if [ ! -d "$1" ]
	then
		echo "Directory \"$1\" doesn't exists" 
		exit 1
	fi
	PWD_=$( pwd )
	cd $1
	COMMON_UTILS_CHECK_DIR=$( pwd )
	cd "$PWD_"
}


function check_file {
	if [ ! -e "$1" ]
	then
		echo "File \"$1\" doesn't exists" 
		exit 1
	fi
}


# Returns "-dirty" if the current git branch is dirty.
function evil_git_dirty {
  [[ $(git diff --shortstat 2> /dev/null | tail -n1) != "" ]] && echo "-dirty"
}

#return absolute filename (/..)
function get_abs_filename {
  # $1 : relative filename
  if [ -d "$(dirname "$1")" ]; then
    echo "$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
  fi
}
