# by default, it generates changlogs from latest-tag to HEAD
function get_changelog_start_end() {
    if [ -z "$CHANGELOG_START" ]; then
        CHANGELOG_START=$(git describe --tags --abbrev=0)
    fi
    if [ -z "$CHANGELOG_START" ]; then
        echo "cannot decide CHANGELOG_START"
        exit 1
    fi

    if [ -z "$CHANGELOG_END" ]; then
        CHANGELOG_END=$(git log --format="%H" -1 HEAD)
    fi
}

function get_author_sign() {
    if [ -z "$AUTHOR_SIGN" ]; then
        AUTHOR_SIGN=$(git var GIT_COMMITTER_IDENT |sed 's/>.*/>/')
    fi
    if [ -z "$AUTHOR_SIGN" ]; then
        echo "unkonwn AUTHOR_SIGN"
        exit 1
    fi
}

function get_changelog_file_name() {
    local file_base_name="changelog.${DIST_ANOLIS_VERSION}"
    local files_num=$(ls ${DIST_CHANGELOG} | grep -E '[0-9]+-changelog.*' | wc -l)
    local file_name=$(printf "%03d-${file_base_name}\n" ${files_num})
    CHANGELOG_FILE=${DIST_CHANGELOG}/${file_name}
}

function generate_changelog() {
    get_changelog_start_end
    get_author_sign
    get_changelog_file_name

    touch ${CHANGELOG_FILE}
    echo "* $(date +"%a %b %d %Y") ${AUTHOR_SIGN} [${DIST_ANOLIS_VERSION}%%DIST%%]" > ${CHANGELOG_FILE}

    # TODO:
    # 1. if config changes, add kernel config refresh log
    # 2. if linux upstream kernel version updated, add related log

    local commits=$(git rev-list ${CHANGELOG_START}..${CHANGELOG_END})
    for commit in $commits
    do
        ## eg: - anolis: net/netfilter: rename nft_expr_info (Kangjie Xu)
        local log=$(git log --format='- %s (%an)' -1 ${commit})

        ## eg: {CVE-2022-32250}
        ## xargs is used to strip space
        local cve_list=$(git log --format='%b' -1 ${commit} | grep -Eio '^[[:blank:]]*Fixes:[[:blank:]]*CVE-.*[[:blank:]]*$' | sed 's/fixes://ig' | xargs | sed 's/[[:blank:]]/,/')
        local cve_fmt=""
        if [ -n "${cve_list}" ]; then
            cve_fmt=$(cat <<< "${cve_list}" | paste -sd "," -)
            cve_fmt=" {${cve_fmt}}"
        fi
        ## merge them together, eg: - anolis: net/netfilter: rename nft_expr_info (Kangjie Xu) {CVE-2022-32250}
        echo "${log}${cve_fmt}" >> ${CHANGELOG_FILE}
    done
    echo "" >> ${CHANGELOG_FILE}
}

generate_changelog