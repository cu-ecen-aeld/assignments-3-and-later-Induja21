#!/bin/sh
# This script prints the info on number of files and number of matching lines in the corresponding file directories
# Author: Induja Narayanan <Induja.Narayanan@colorado.edu>

checkIfInputsToScriptAreAvailable()
{
    #Check if the arguments are NULL charcters, if so no arguments are given as input
    #Throw error message and exit

    if [ -z "$1" ] || [ -z "$2" ]; then
        echo -e "Error: Please provide the missing arguments \nExample: ./finder.sh <filesdir> <searchstr>  \nfilesdir: The directory path that needs to be checked \nsearchstr: The string information that needs to be checked in the provided directory path"
        exit 1
    fi
}

checkIfInputDirectoryIsValid()
{
    #Check if the given directory exist else exit with error value 1 and an error message
    dirName=$(dirname "$1")
    if [ ! -d "$dirName" ];then
        echo "Error: Provided directory does not exist. Please enter an existing directory"
        exit 1
    else
        return 0
    fi
}

findNumberOfFilesAndNumberOfMatchingLines()
{
    filesdir=$1
    searchstr=$2

    #Find the number of files present in the provided directory
    NumOfFiles=$(find $filesdir -type f | wc -l)

    #Find the number of lines having the provided string
    NumOfLines=$(grep -r -o "$searchstr" $filesdir | wc -l)

    #Print the details number of files found and number of lines containing the input string
    echo "The number of files are $NumOfFiles and the number of matching lines are $NumOfLines"
}

#Check if input arguments are proper
checkIfInputsToScriptAreAvailable "$@"

#Check if input directory is valid and if valid find the files and lines containing the matching string
checkIfInputDirectoryIsValid "$1"
isDirectoryValid=$?
if [ $isDirectoryValid -eq 0 ];then
    findNumberOfFilesAndNumberOfMatchingLines "$@"
fi
exit 0