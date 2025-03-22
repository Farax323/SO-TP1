cd /root

alias dcrun="docker run -v ${PWD}:/root --privileged -ti --name SO agodio/itba-so-multi-platform:3.0"
alias dcexec="docker exec -ti SO bash"
alias dcstart="docker start SO"
alias dcstop="docker stop SO"
alias dcrm="docker rm -f SO"

export GCC_COLORS='error=01;31:warning=01;35:note=01;36:caret=01;32:locus=01:quote=01'
alias ls='ls --color'

