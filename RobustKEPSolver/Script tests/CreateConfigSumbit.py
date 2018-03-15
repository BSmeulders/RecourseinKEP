# Script for generating config file and submit file to test the RKEP solver

import sys
import glob
import argparse
import os

# Globale variables to store parameters given to the script to
# write the config and submit files accordingly

# Path separator changes according to OS
sep = "/"

# Path to the directory containing the instance files to test
dirPath = ""

# Memory limit for the cluster
memLimitCluster = 2048

nrScenarios = 100

solverType = 5

formulation = 1

timeLimitCplex = 7200

memLimitCplex = 7500

# Max number of tests for each instance. This is fixed and is gathered from the subset recourse solution files.
maxTests = {'V-SR-50-0.6-4-0-50-Said-9.txt': '64', 'V-SR-50-0.6-4-0-50-Said-7.txt': '43', 'V-SR-50-0.6-4-0-50-Said-2.txt': '65', 'V-SR-50-0.2-4-0-50-Said-2.txt': '72', 'V-SR-50-0.6-4-0-50-Said-4.txt': '55', 'V-SR-50-0.6-4-0-50-Said-3.txt': '47', 'V-SR-50-0.8-4-0-50-Said-4.txt': '53', 'V-SR-50-0.2-4-0-50-Said-10.txt': '43', 'V-SR-50-0.2-4-0-50-Said-1.txt': '59', 'V-SR-50-0.4-4-0-50-Said-9.txt': '61', 'V-SR-50-0.8-4-0-50-Said-1.txt': '35', 'V-SR-50-0.2-4-0-50-Said-4.txt': '55', 'V-SR-50-0.6-4-0-50-Said-1.txt': '57', 'V-SR-50-0.6-4-0-50-Said-6.txt': '46', 'V-SR-50-0.8-4-0-50-Said-8.txt': '88', 'V-SR-50-0.4-4-0-50-Said-8.txt': '62', 'V-SR-50-0.6-4-0-50-Said-5.txt': '75', 'V-SR-50-0.8-4-0-50-Said-3.txt': '69', 'V-SR-50-0.4-4-0-50-Said-7.txt': '80', 'V-SR-50-0.2-4-0-50-Said-8.txt': '60', 'V-SR-50-0.4-4-0-50-Said-4.txt': '78', 'V-SR-50-0.4-4-0-50-Said-10.txt': '58', 'V-SR-50-0.2-4-0-50-Said-6.txt': '64', 'V-SR-50-0.2-4-0-50-Said-9.txt': '66', 'V-SR-50-0.6-4-0-50-Said-8.txt': '76', 'V-SR-50-0.2-4-0-50-Said-5.txt': '38', 'V-SR-50-0.8-4-0-50-Said-2.txt': '65', 'V-SR-50-0.2-4-0-50-Said-3.txt': '70', 'V-SR-50-0.4-4-0-50-Said-1.txt': '50', 'V-SR-50-0.4-4-0-50-Said-5.txt': '74', 'V-SR-50-0.8-4-0-50-Said-5.txt': '59', 'V-SR-50-0.4-4-0-50-Said-3.txt': '28', 'V-SR-50-0.8-4-0-50-Said-7.txt': '48', 'V-SR-50-0.6-4-0-50-Said-10.txt': '75', 'V-SR-50-0.4-4-0-50-Said-6.txt': '60', 'V-SR-50-0.2-4-0-50-Said-7.txt': '54', 'V-SR-50-0.4-4-0-50-Said-2.txt': '48', 'V-SR-50-0.8-4-0-50-Said-6.txt': '71', 'V-SR-50-0.8-4-0-50-Said-9.txt': '51', 'V-SR-50-0.8-4-0-50-Said-10.txt': '70'}

# Switch number used to design formulation and solver type to name the files correctly
switchForm = {1: 'Dickerson', 2: 'Cycle', 3: 'EE'}
switchSolver = {5: 'B', 6: 'LP'}

# Path of the directory containing the generated files. Can not be set until
# the instance files path has been retrieved
configPath = ""
submitPath = ""

# Mail adress for cluster setting (?)
mailCluster = "bart.smeulders@ulg.ac"

# Name of the executable to be tested
RKEPbin = "RobustKEP"

# Parsing arguments for config and submits files
def parseArguments():
    global dirPath, nrScenarios, solverType, formulation, timeLimitCplex, memLimitCplex, memLimitCluster, mailCluster, RKEPbin
    parser = argparse.ArgumentParser(description='Generates configuration and submission file for RKEP tests')
    parser.add_argument('--directory', '-dir', action='store', type=str, required=True, help="[REQUIRED] path (relative or absolute) to directory containing tests instances.")
    parser.add_argument('--nrscenarios', '-nrs', action='store', type=int, default = 100, help="Number of scenarios. Default = 100")
    parser.add_argument('--solvertype', '-st', action='store', type=int, default = 5, help="Solver type. Default = 5 (Benders)")
    parser.add_argument('--formulation', '-form', action='store', type=int, default = 1, help="KEP formulation. Default = 1")
    parser.add_argument('--cplextime', '-cplt', action='store', type=int, default = 7200, help="CPLEX time limit. Default = 7200 s")
    parser.add_argument('--cplexmem', '-cplm', action='store', type=int, default = 7500, help="CPLEX memory limit. Default = 7500 mb")
    parser.add_argument('--clustermem', '-clusm', action='store', type=int, default = 8192, help="Memory limit in submission file. Default = 8192")
    parser.add_argument('--mail', '-m', action='store', type=str, default = "bart.smeulders@ulg.ac", help="Mail adress for cluster settings. Default = bart.smeulders@ulg.ac")
    parser.add_argument('--bin', '-b', action='store', type=str, default = "RobustKEP", help="Name of the binary to launch the tests on. Default = RobustKEP")
    args = parser.parse_args()
    dirPath = os.path.abspath(args.directory)
    nrScenarios = args.nrscenarios
    solverType = args.solvertype
    formulation = args.formulation
    timeLimitCplex = args.cplextime
    memLimitCplex = args.cplexmem
    memLimitCluster = args.clustermem
    mailCluster = args.mail
    RKEPbin = args.bin


# Return the instance number and probability associated with an instance file "V-50...-Said-10.txt"
def getProbNumber(instanceName):
    instance = instanceName.split(".txt")[0]
    instanceNumber = instance.split("-")[-1]
    instanceProb = instance.split("-")[2]
    return instanceProb, instanceNumber

# Convert the name of an instance file to the name of a SR solution file to gather the
# maximum number of tests. It uses the instance number Said-x and the probability of success
def nrMaxTests(instanceName):
    instanceProb, instanceNumber = getProbNumber(instanceName)
    SRName = "V-SR-50-" + instanceProb + "-4-0-50-Said-" + instanceNumber + ".txt"
    return maxTests[SRName]

# Creates the subdirectories to store config and submit files
def createSubdir():
    global configPath, submitPath
    if dirPath[-1] == '/' or dirPath[-1] == '\\':
        configPath = dirPath + "Config"
        sumbitPath = dirPath + "Submit"
    else:
        configPath = dirPath + sep + "Config"
        submitPath = dirPath + sep + "Submit"
    if not os.path.exists(configPath): os.makedirs(configPath)
    if not os.path.exists(submitPath): os.makedirs(submitPath)


def getOutputName(instanceName):
    instanceProb, instanceNumber = getProbNumber(instanceName)
    return "V-" + switchSolver[solverType] + "-" + switchForm[formulation] \
    + "-50-" + instanceProb + "-3-1-" + str(nrScenarios) + "-Said-" + str(instanceNumber) + ".txt"

# Creates the config file to launch instanceName. Assumes that the
# directory to store config files exists.
def createConfigFile(instanceName):
    configFileName = "Config-" + instanceName
    configFile = open(configPath + sep + configFileName , "w")
    configFile.truncate(0) # Clean the files if it already exists
    configFile.write("Cyclelength = 3\n")
    configFile.write("Chainlength = 3\n")
    configFile.write("Failure Type = 2\n")
    configFile.write("Solver = " + str(solverType) + "\n")
    configFile.write("Formulation = " + str(formulation) + "\n")
    configFile.write("# Expected Type = 1\n")
    configFile.write("Max Test = " + nrMaxTests(instanceName) + "\n")
    configFile.write("Scenarios = " + str(nrScenarios) + "\n")
    configFile.write("# Subset Recourse Size = 1\n")
    configFile.write("Input Type = 2\n")
    configFile.write("Input File = " + instanceName + "\n")
    configFile.write("Testvar Output = " + getOutputName(instanceName) + "\n")
    configFile.write("# Solution Input File = " + getOutputName(instanceName) + "\n")
    configFile.write("Time Limit = " + str(timeLimitCplex) + "\n")
    configFile.write("Memory Limit = " + str(memLimitCplex) + "\n")
    configFile.write("# Limited World = 1")
    configFile.close()
    return configFileName

def createSubmitFile(instanceName, configFileName):
    instanceProb, instanceNumber = getProbNumber(instanceName)
    submitFile = open(submitPath + sep + "Submit-" + instanceName[:-4] + ".sh", "w")
    submitFile.truncate(0)
    submitFile.write("#!/bin/bash\n\n")
    submitFile.write("#SBATCH --job-name=" + str(instanceNumber) + "-" + str(instanceProb) + switchSolver[solverType][0] + switchForm[formulation][0] + "\n")
    submitFile.write("#SBATCH --time=03:00:00\n")
    submitFile.write("#\n")
    submitFile.write("#SBATCH --output=Out-" + getOutputName(instanceName) + "\n")
    submitFile.write("#SBATCH --ntasks=1\n")
    submitFile.write("#SBATCH --cpus-per-task=4\n")
    submitFile.write("#SBATCH --mem-per-cpu=" + str(memLimitCluster) + "\n")
    submitFile.write("#SBATCH --partition=Def\n")
    submitFile.write("#\n")
    submitFile.write("#SBATCH --mail-user=" + mailCluster + "\n")
    submitFile.write("#SBATCH --mail-type=ALL\n\n")
    submitFile.write("export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK\n")
    submitFile.write("./" + RKEPbin + " " + configFileName)
    submitFile.close()

if __name__ == '__main__':
    if os.name == 'nt': sep = "\\"
    parseArguments()
    createSubdir()
    instanceList = glob.glob(dirPath + "/V-*.txt")
    for instance in instanceList:
        instanceName = instance.split(sep)[-1]
        configFileName = createConfigFile(instanceName)
        createSubmitFile(instanceName, configFileName)
