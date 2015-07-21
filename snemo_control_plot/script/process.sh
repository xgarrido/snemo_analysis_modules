#!/usr/bin/zsh -i


function run()
{
    __pkgtools__at_function_enter run

    # Lambda function
    --properly-exit() {
        aggregator goto falaise
        pkgtools__quietly_run "git checkout master"
        pkgtools__quietly_run "aggregator reinstall falaise"
    }

    aggregator goto falaise

    pkgtools__msg_notice "Checking out $1 revision"
    if  [ "$1" = "master" ]; then
        pkgtools__quietly_run "git checkout master"
    else
        local hash_commit=$(git svn find-rev r$1)
        if [ -z ${hash_commit} ]; then
            pkgtools__msg_error "Checking out $1 revision fails !"
            --properly-exit
            __pkgtools__at_function_exit
            return 1
        fi
        pkgtools__quietly_run "git checkout ${hash_commit}"
    fi

    pkgtools__msg_notice "Building $1 revision"
    pkgtools__quietly_run "aggregator reinstall falaise"
    if $(pkgtools__last_command_fails); then
        pkgtools__msg_error "Building $1 revision fails !"
        --properly-exit
        __pkgtools__at_function_exit
        return 1
    fi

    pkgtools__msg_notice "Running $1 revision"
    pkgtools__quietly_run "mydpp_processing --module channel_chain -M 10 -% 1 -P notice"
    local config_path=$SNAILWARE_SIMULATION_DIR/snemo_analysis_modules/config
    pkgtools__quietly_run \
        "bxdpp_processing --module snemo_control_plot              \
        --module-manager-config ${config_path}/module_manager.conf \
        --dlls-config ${config_path}/dlls.conf                     \
        -i /tmp/garrido/snemo.d/io_output_analysed.brio"
    pkgtools__quietly_run "cp /tmp/garrido/snemo.d/snemo_control_plot_histos.root ${build_dir}/root/snemo_control_plot_histos_r$1.root"

    --properly-exit
    __pkgtools__at_function_exit
    return 0
}

function compare()
{
    __pkgtools__at_function_enter compare

    pkgtools__msg_notice "Comparing $1 with $2 revision"

    (
        cd ${build_dir}
        pkgtools__quietly_run \
            "rpu --histogram-name \".*\"                                       \
             --reference-root-file ${build_dir}/root/snemo_control_plot_histos_r$1.root \
             --root-files          ${build_dir}/root/snemo_control_plot_histos_r$2.root \
             --show-ratio --fill-reference --colors \"#FFFF00,#000000\"            \
             --save-directory ${build_dir}/figures"

        cd figures
        pkgtools__quietly_run "ls *.tex | parallel tikz2pdf -o -q --xkcd {}"
    )

    __pkgtools__at_function_exit
    return 0
}

function generate_org_file()
{
    __pkgtools__at_function_enter generate_org_file

    # Lambda functions
    --push() {
        cat << EOF >> ${org_file}
$1

EOF
    }
    --generate-section() {
        --push "${section}"
        for pdf in figures/${regex}*.pdf; do
            --push "[[file:./$pdf]]"
        done
    }

    (
        cd ${build_dir}
        local org_file=README.org
        test -f ${org_file} && rm -f ${org_file}
        touch ${org_file}
        cat <<EOF>> ${org_file}
#+TITLE:   SuperNEMO control plots
#+AUTHOR:  Xavier Garrido
#+EMAIL:   xavier.garrido@lal.in2p3.fr
#+DATE:    $(date +%Y-%m-%d)
#+OPTIONS: ^:{} num:nil toc:t

EOF
        # org-mode seems not to like double semi-colon in filename
        (
            cd figures
            for f in *::*.pdf; do
                local newfile=$(echo $f | sed -e 's/::/_/g')
                mv $f $newfile
            done
        )
        (
            section="* Simulated data plots"
            description=
            regex=SD
            --generate-section
        )
        (
            section="* Calibrated data plots"
            description=
            regex=CD
            --generate-section
        )
        (
            section="* Tracker clustering data plots"
            description=
            regex=TCD
            --generate-section
        )
        (
            section="* Tracker trajectory data plots"
            description=
            regex=TTD
            --generate-section
        )
        (
            section="* Topology data plots"
            description=
            regex=TD
            --generate-section
        )

        pkgtools__msg_notice "Generate html documentation"
        org-pages --html --debug generate
        pkgtools__msg_notice "Generate pdf documentation"
        org-pages --pdf --debug generate
    )

    __pkgtools__at_function_exit
    return 0
}

ref_revision=
dev_revision=
opwd=$(pwd)
build_dir=$(pwd)/build

# Set logging default values
__pkgtools__default_values

while [ -n "$1" ]; do
    token="$1"
    if [ "${token}" = "-h" -o "${token}" = "--help" ]; then
        return 0
    elif [ "${token}" = "-d" -o "${token}" = "--debug" ]; then
	pkgtools__msg_using_debug
    elif [ "${token}" = "-D" -o "${token}" = "--devel" ]; then
	pkgtools__msg_using_devel
    elif [ "${token}" = "-v" -o "${token}" = "--verbose" ]; then
	pkgtools__msg_using_verbose
    elif [ "${token}" = "-W" -o "${token}" = "--no-warning" ]; then
	pkgtools__msg_not_using_warning
    elif [ "${token}" = "-q" -o "${token}" = "--quiet" ]; then
	pkgtools__msg_using_quiet
	export PKGTOOLS_MSG_QUIET=1
    elif [ "${token}" = "--ref-revision" ]; then
        shift 1
        ref_revision=$1
    elif [ "${token}" = "--dev-revision" ]; then
        shift 1
        dev_revision=$1
    fi
    shift 1
done

if [ -z ${dev_revision} -o -z ${ref_revision} ]; then
    pkgtools__msg_error "Missing svn revision !"
fi

pkgtools__msg_devel "Reference revision ${ref_revision}"
pkgtools__msg_devel "Dev. revision ${dev_revision}"

# test -d ${build_dir} && rm -fr ${build_dir}; mkdir -p ${build_dir}/{root,figures}

# run ${ref_revision}
# run ${dev_revision}

# compare ${ref_revision} ${dev_revision}

generate_org_file

cd ${opwd}

exit 0

# end
