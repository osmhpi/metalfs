#!/bin/bash

version=1.0
program=`basename "$0"`

# output formatting
bold=$(tput bold)
normal=$(tput sgr0)

# Print usage message helper function
function usage() {
  echo "Usage: ${program} [OPTIONS]"
  echo "    [-w <wrapper>]       HDL wrapper name."
  echo "    [-p <part_number>]   FPGA part number."
  echo "    [-f <files>]         files to be used."
  echo "    [-V] Print program version (${version})"
  echo "    [-h] Print this help message."
  echo
  echo "Utility to generate vivado tcl configuration file."
  echo
}

# Parse any options given on the command line
while getopts ":w:p:f:Vh" opt; do
  case ${opt} in
      w)
      wrapper=$OPTARG
      ;;
      p)
      part_number=$OPTARG
      ;;
      f)
      files=$OPTARG
      ;;
      V)
      echo "${version}" >&2
      exit 0
      ;;
      h)
      usage;
      exit 0
      ;;
      \?)
      printf "${bold}ERROR:${normal} Invalid option: -${OPTARG}\n" >&2
      exit 1
      ;;
      :)
      printf "${bold}ERROR:${normal} Option -$OPTARG requires an argument.\n" >&2
      exit 1
      ;;
  esac
done

shift $((OPTIND-1))
# now do something with $@

#### TCL config script for vivado #########################################
cat <<EOF
set src_dir     action_ip_prj/action_ip_prj.srcs/sources_1/ip

## Create a new Vivado IP Project
create_project -force managed_ip_project managed_ip_project -part ${part_number}

set_property target_language    VHDL [current_project]
set_property simulator_language VHDL [current_project]
set_property target_simulator   XSim [current_project]

# Can that be a list?
foreach file [ list ${files} ] {
  add_files -norecurse ${PWD}/\${file}
}

update_compile_order -fileset sources_1

ipx::package_project -root_dir ip_user_files -vendor user.org -library user -taxonomy /UserIP -import_files

ipx::create_xgui_files [ipx::current_core]
ipx::update_checksums [ipx::current_core]
ipx::save_core [ipx::current_core]

close_project
EOF
