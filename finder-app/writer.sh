#!/bin/bash
# This script writes the user input string to the user input file.
# Author: Induja Narayanan <Induja.Narayanan@colorado.edu>

checkIfInputsToScriptAreAvailable()
{
    #Check if the arguments are NULL charcters, if so no arguments are given as input
    #Throw error message and exit
    if [ -z "$1" ] || [ -z "$2" ]; then
        echo -e "Error: Please provide the missing arguments \nExample: ./writer.sh <writefile> <writestr>  \nwritefile: The file that needs to be created \nwritestr: Contents of the string to be written to the file"
        exit 1
    fi
}

checkIfInputDirectoryIsValid()
{
    #Check if the given directory exist else create a directory
    dirName=$(dirname "$1")
    if [ ! -d "$dirName" ];then
        echo "Directory doesn't exist. Hence creating a directory"
        mkdir -p $dirName
    fi   
}

createAndWriteContentsToTheFile()
{
    writeFile=$1
    writeStr=$2

    #Write input contents to the given file
    echo $writeStr > $writeFile

    #If any error exist , throw error message and print error status
    errorStatus=$?
    if [ $errorStatus -ne 0 ];then
        echo "File creation unsuccessful.Errno: $errorStatus"
        exit 1
    else
        echo "File Creation Successful"
    fi
}
#Check if input arguments are proper
checkIfInputsToScriptAreAvailable "$@"

#Check if input directory is present else create a directory
checkIfInputDirectoryIsValid "$@"

#Create And Write Contents to the file
createAndWriteContentsToTheFile "$@"
exit 0