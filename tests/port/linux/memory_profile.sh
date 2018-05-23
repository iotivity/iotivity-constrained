#!/bin/bash
# ****************************************************************************
# *
# * Copyright 2018 Samsung Electronics All Rights Reserved.
# *
# * Licensed under the Apache License, Version 2.0 (the "License");
# * you may not use this file except in compliance with the License.
# * You may obtain a copy of the License at
# *
# * http://www.apache.org/licenses/LICENSE-2.0
# *
# * Unless required by applicable law or agreed to in writing,
# * software distributed under the License is distributed on an
# * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
# * either express or implied. See the License for the specific
# * language governing permissions and limitations under the License.
# *
# ****************************************************************************/
#required tool
#sudo apt-get install valgrind
#sudo apt-get install massif-visualizer
#
#usages
#./memory_profile.sh -mt -mp <sampleapp>
#it will generate memory profile file on memory_profile/massif.out.*
#download and open with massif-visualizer to view memory usage.

PrintUsage()
{
  echo
  echo -e "Memory Profile script usage:"
  echo -e "$ $0 -mt -mp <sampleappname>"
  echo
  echo -e "    -mt: wil memory trace of given sampleapp"
  echo -e "    -mp: wil make memory profile of given sampleapp"
  echo
  exit
}

doReadFromFile()
{
	tail "${iotivity}/memory_profile/$memoryfile"
}

doMemoryProfile()
{
	for var in "${POSITIONAL[@]}"
	do
		valgrind --tool=massif --time-unit=B --stacks=yes --heap=yes ./$var
		massiffile=$(echo "$var" | tr '/' ',').massif.log.txt

		ms_print massif.out.* >> "${iotivity}/memory_profile/$massiffile"
		memoryfile=$(echo "$var" | tr '/' ',').memory.log.txt
		awk '/KB/{getline;gsub("[:^]","");print $1}' "${iotivity}/memory_profile/$massiffile" >> "${iotivity}/memory_profile/$memoryfile"

		mv massif.out.* "${iotivity}/memory_profile/"
	done
}

doMemoryTrace()
{
	for var in "${POSITIONAL[@]}"
	do
		make cleanall
		make MEMTRACE=1
		make test MEMTRACE=1
		unrechablememoryfile=$(echo "$var" | tr '/' ',').unrechablememoryfile.log.txt
		./$var >> "${iotivity}/memory_profile/$unrechablememoryfile"
		
	done
}

POSITIONAL=()
MEMTRACE="no"
MEMPROFILE="no"
while [[ $# -gt 0 ]]
do
key="$1"

case $key in
    h|-h|help|-help|--help|?)  PrintUsage;;
    -mt|--memorytrace)
    MEMTRACE="yes"
    shift # past argument
    ;;
    -mp|--memoryprofile)
    MEMPROFILE="yes"
    shift # past argument
    ;;
    --default)
    DEFAULT=YES
    shift # past argument
    ;;
    *)    # unknown option
    POSITIONAL+=("$1") # save it in an array for later
    shift # past argument
    ;;
esac
done
set -- "${POSITIONAL[@]}" # restore positional parameters

echo MEMTRACE  = "${MEMTRACE}"
echo MEMPROFILE     = "${MEMPROFILE}"

iotivity="$PWD"

root_dir="${iotivity}/../../.."
linux_dir="${root_dir}/port/linux/"
cd "$linux_dir"

#for script run need to remove previous file
#rm -rf "${iotivity}/memory_profile"
#rm -rf massif.out.*

mkdir "${iotivity}/memory_profile"

if [ ${MEMPROFILE} == 'yes' ]
then
	doMemoryProfile
fi

# make with memtrace to get unreachable memory with the address function
if [ ${MEMTRACE} == 'yes' ]
then
	doMemoryTrace
fi

