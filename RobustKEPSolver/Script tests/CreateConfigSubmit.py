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

cycleLength = 3

chainLength = 3

failureType = 2

# Memory limit for the cluster
memLimitCluster = 2048

timeLimitCluster = "3:00:00"

nrScenarios = 100

solverType = 5

formulation = 1

timeLimitCplex = 7200

memLimitCplex = 7500

nrCpuCluster = 4

testType = 1

# Max number of tests for each instance. This is fixed and is gathered from the subset recourse solution files.
maxTests = {'V-SR-50-0.6-4-0-50-Said-9.txt': '64', 'V-SR-50-0.6-4-0-50-Said-7.txt': '43', 'V-SR-50-0.6-4-0-50-Said-2.txt': '65', 'V-SR-50-0.2-4-0-50-Said-2.txt': '72', 'V-SR-50-0.6-4-0-50-Said-4.txt': '55', 'V-SR-50-0.6-4-0-50-Said-3.txt': '47', 'V-SR-50-0.8-4-0-50-Said-4.txt': '53', 'V-SR-50-0.2-4-0-50-Said-10.txt': '43', 'V-SR-50-0.2-4-0-50-Said-1.txt': '59', 'V-SR-50-0.4-4-0-50-Said-9.txt': '61', 'V-SR-50-0.8-4-0-50-Said-1.txt': '35', 'V-SR-50-0.2-4-0-50-Said-4.txt': '55', 'V-SR-50-0.6-4-0-50-Said-1.txt': '57', 'V-SR-50-0.6-4-0-50-Said-6.txt': '46', 'V-SR-50-0.8-4-0-50-Said-8.txt': '88', 'V-SR-50-0.4-4-0-50-Said-8.txt': '62', 'V-SR-50-0.6-4-0-50-Said-5.txt': '75', 'V-SR-50-0.8-4-0-50-Said-3.txt': '69', 'V-SR-50-0.4-4-0-50-Said-7.txt': '80', 'V-SR-50-0.2-4-0-50-Said-8.txt': '60', 'V-SR-50-0.4-4-0-50-Said-4.txt': '78', 'V-SR-50-0.4-4-0-50-Said-10.txt': '58', 'V-SR-50-0.2-4-0-50-Said-6.txt': '64', 'V-SR-50-0.2-4-0-50-Said-9.txt': '66', 'V-SR-50-0.6-4-0-50-Said-8.txt': '76', 'V-SR-50-0.2-4-0-50-Said-5.txt': '38', 'V-SR-50-0.8-4-0-50-Said-2.txt': '65', 'V-SR-50-0.2-4-0-50-Said-3.txt': '70', 'V-SR-50-0.4-4-0-50-Said-1.txt': '50', 'V-SR-50-0.4-4-0-50-Said-5.txt': '74', 'V-SR-50-0.8-4-0-50-Said-5.txt': '59', 'V-SR-50-0.4-4-0-50-Said-3.txt': '28', 'V-SR-50-0.8-4-0-50-Said-7.txt': '48', 'V-SR-50-0.6-4-0-50-Said-10.txt': '75', 'V-SR-50-0.4-4-0-50-Said-6.txt': '60', 'V-SR-50-0.2-4-0-50-Said-7.txt': '54', 'V-SR-50-0.4-4-0-50-Said-2.txt': '48', 'V-SR-50-0.8-4-0-50-Said-6.txt': '71', 'V-SR-50-0.8-4-0-50-Said-9.txt': '51', 'V-SR-50-0.8-4-0-50-Said-10.txt': '70'}

# Switch number used to design formulation and solver type to name the files correctly
switchForm = {1: 'Dickerson', 2: 'EE', 3: 'Cycle'}
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
    global dirPath, nrScenarios, solverType, formulation, timeLimitCplex, memLimitCplex, memLimitCluster, mailCluster, RKEPbin, timeLimitCluster
    global cycleLength, chainLength, failureType, nrCpuCluster, testType
    parser = argparse.ArgumentParser(description='Generates configuration and submission file for RKEP tests')
    parser.add_argument('--directory', '-dir', action='store', type=str, required=True, help="[REQUIRED] path (relative or absolute) to directory containing tests instances.")
    parser.add_argument('--nrscenarios', '-nrs', action='store', type=int, default = 100, help="Number of scenarios. Default = 100")
    parser.add_argument('--cycle', '-cy', action='store', type=int, default = 3, help="Maximum cycle length. Default = 3")
    parser.add_argument('--chain', '-ch', action='store', type=int, default = 3, help="Maximum chain length. Default = 3")
    parser.add_argument('--failure', '-f', action='store', type=int, default = 2, help="Failure type for scenarios. Default = 2 (Vertex only)")
    parser.add_argument('--solvertype', '-st', action='store', type=int, default = 5, help="Solver type. Default = 5 (Benders)")
    parser.add_argument('--formulation', '-form', action='store', type=int, default = 1, help="KEP formulation. Default = 1")
    parser.add_argument('--cplextime', '-cplt', action='store', type=int, default = 7200, help="CPLEX time limit. Default = 7200 s")
    parser.add_argument('--cplexmem', '-cplm', action='store', type=int, default = 7500, help="CPLEX memory limit. Default = 7500 mb")
    parser.add_argument('--clustermem', '-clusm', action='store', type=int, default = 8192, help="Memory limit in submission file. Default = 8192 mb")
    parser.add_argument('--clustertime', '-clust', action='store', type=str, default = "03:00:00", help="Time limit in submission file. Format: hh:mm:ss. Default = 03:00:00")
    parser.add_argument('--mail', '-m', action='store', type=str, default = "bart.smeulders@ulg.ac", help="Mail adress for cluster settings. Default = bart.smeulders@ulg.ac")
    parser.add_argument('--bin', '-b', action='store', type=str, default = "RobustKEP", help="Name of the binary to launch the tests on. Default = RobustKEP")
    parser.add_argument('--testtype', '-ttype', action='store', type=int, default = 1, help="Type of post-ex evaluation 1 = exact, 2 = monte-carlo. Default = 1")
    args = parser.parse_args()
    dirPath = os.path.abspath(args.directory)
    nrScenarios = args.nrscenarios
    chainLength = args.chain
    cycleLength = args.cycle
    failureType = args.failure
    solverType = args.solvertype
    formulation = args.formulation
    timeLimitCplex = args.cplextime
    memLimitCplex = args.cplexmem
    memLimitCluster = args.clustermem
    timeLimitCluster = args.clustertime
    mailCluster = args.mail
    RKEPbin = args.bin
    testType = args.testtype
    if solverType == 6: nrCpuCluster = 1 # If exact expected transplant computation only one CPU is needed


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

# Name of the output file containing the solutions (beta arcs)
def getOutputName(instanceName):
    instanceProb, instanceNumber = getProbNumber(instanceName)
    return "V-" + switchSolver[solverType] + "-" + switchForm[formulation] \
    + "-50-" + instanceProb + "-3-1-" + str(nrScenarios) + "-Said-" + str(instanceNumber) + ".txt"

# Used only to compute exact expected transplants given a solution file (post-ex evaluation)
# we need, given a solution file, to find the instance file it has been computed from.
def identifyInstance(solutionFileName):
    instanceNb = solutionFileName[-5]
    if (instanceNb == '0'): instanceNb = "10" # Instance number goes from 1 to 10
    pr = solutionFileName.split("-")[4]
    return "V-50-" + pr + "-Said-" + instanceNb + ".txt"


# Creates the config file to launch instanceName. Assumes that the
# directory to store config files exists.
def createConfigFile(instanceName):
    configFileName = "Config-" + instanceName[:-4] + "-" + switchSolver[solverType][0] + "-" + switchForm[formulation][0] + "-" + str(nrScenarios) + ".txt"
    configFile = open(configPath + sep + configFileName , "w")
    configFile.truncate(0) # Clean the files if it already exists
    configFile.write("Cyclelength = " + str(cycleLength) + "\n")
    configFile.write("Chainlength = " + str(chainLength) + "\n")
    configFile.write("Failure Type = " + str(failureType) + "\n")
    configFile.write("Solver = " + str(solverType) + "\n")
    configFile.write("Formulation = " + str(formulation) + "\n")
    configFile.write("Expected Type = " + str(testType) + "\n")
    configFile.write("Max Test = " + nrMaxTests(instanceName) + "\n")
    configFile.write("Scenarios = " + str(nrScenarios) + "\n")
    configFile.write("# Subset Recourse Size = 1\n")
    configFile.write("Input Type = 2\n")
    configFile.write("Input File = " + instanceName + "\n")
    configFile.write("Testvar Output = " + getOutputName(instanceName) + "\n")
    configFile.write("Solution Input File = " + getOutputName(instanceName) + "\n")
    configFile.write("Time Limit = " + str(timeLimitCplex) + "\n")
    configFile.write("Memory Limit = " + str(memLimitCplex) + "\n")
    configFile.write("# Limited World = 1")
    configFile.close()
    return configFileName

def createSubmitFile(instanceName, configFileName):
    instanceProb, instanceNumber = getProbNumber(instanceName)
    submitFileName = "Submit-" + instanceName[:-4] + "-" + switchSolver[solverType][0] + "-" + switchForm[formulation][0] + "-" + str(nrScenarios)  + ".sh"
    submitFile = open(submitPath + sep + submitFileName, "w")
    submitFile.truncate(0)
    submitFile.write("#!/bin/bash\n\n")
    submitFile.write("#SBATCH --job-name=" + str(instanceNumber) + "-" + str(instanceProb) + switchSolver[solverType][0] + switchForm[formulation][0] + "\n")
    submitFile.write("#SBATCH --time=" + timeLimitCluster + "\n")
    submitFile.write("#\n")
    prefix = "Out-"
    if solverType == 6: prefix = "ExPost-"
    submitFile.write("#SBATCH --output=" + prefix + getOutputName(instanceName) + "\n")
    submitFile.write("#SBATCH --ntasks=1\n")
    submitFile.write("#SBATCH --cpus-per-task=" + str(nrCpuCluster) + "\n")
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
    if solverType == 5:
        for instance in instanceList:
            instanceName = instance.split(sep)[-1]
            configFileName = createConfigFile(instanceName)
            createSubmitFile(instanceName, configFileName)
    if solverType == 6:
        for solutionFile in instanceList:
            solutionFileName = solutionFile.split(sep)[-1]
            instanceFileName = identifyInstance(solutionFileName)
            configFileName = createConfigFile(instanceFileName)
            createSubmitFile(instanceFileName, configFileName)
