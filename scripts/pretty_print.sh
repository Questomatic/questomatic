pp_step_cnt=0

function pp_init {
    pp_step_cnt=0 
}

function pp_info {
    echo -e "[${pp_step_cnt}]\e[1m$1\e[0m"
    pp_step_cnt=$((pp_step_cnt+1))
}

function pp_step {
    echo -e "[${pp_step_cnt}]\e[32m\e[1m$1\e[0m"
    pp_step_cnt=$((pp_step_cnt+1))
}

function pp_warn {
    echo -e "[${pp_step_cnt}]\e[33m\e[1m$1\e[0m"
    pp_step_cnt=$((pp_step_cnt+1))
}

function pp_error {
    echo -e "[${pp_step_cnt}]\e[31m\e[1m$1\e[0m"
    pp_step_cnt=$((pp_step_cnt+1))
}

function pp_ok {
    echo -e "[${pp_step_cnt}]\e[32m\e[1m$1\e[0m"
    pp_step_cnt=$((pp_step_cnt+1))
}