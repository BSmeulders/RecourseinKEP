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

postEx = 0

warmStart = 0

benderVersion = 0

# Max number of tests for each instance. This is fixed and is gathered from the subset recourse solution files.
maxTests50 = {'V-SR-50-0.4-3-1-Said-2.txt': '44', 'V-SR-50-0.2-3-1-Said-2.txt': '65', 'V-SR-50-0.6-3-1-Said-1.txt': '49', 'V-SR-50-0.6-3-1-Said-2.txt': '67', 'V-SR-50-0.8-3-1-Said-7.txt': '48', 'V-SR-50-0.4-3-1-Said-9.txt': '59', 'V-SR-50-0.2-3-1-Said-3.txt': '62', 'V-SR-50-0.2-3-1-Said-8.txt': '52', 'V-SR-50-0.8-3-1-Said-2.txt': '66', 'V-SR-50-0.4-3-1-Said-4.txt': '77', 'V-SR-50-0.2-3-1-Said-1.txt': '47', 'V-SR-50-0.4-3-1-Said-3.txt': '27', 'V-SR-50-0.4-3-1-Said-7.txt': '72', 'V-SR-50-0.6-3-1-Said-5.txt': '74', 'V-SR-50-0.8-3-1-Said-4.txt': '55', 'V-SR-50-0.8-3-1-Said-1.txt': '37', 'V-SR-50-0.2-3-1-Said-9.txt': '63', 'V-SR-50-0.6-3-1-Said-10.txt': '73', 'V-SR-50-0.2-3-1-Said-4.txt': '52', 'V-SR-50-0.2-3-1-Said-6.txt': '64', 'V-SR-50-0.8-3-1-Said-3.txt': '71', 'V-SR-50-0.6-3-1-Said-9.txt': '61', 'V-SR-50-0.6-3-1-Said-6.txt': '49', 'V-SR-50-0.2-3-1-Said-5.txt': '38', 'V-SR-50-0.2-3-1-Said-7.txt': '51', 'V-SR-50-0.8-3-1-Said-10.txt': '67', 'V-SR-50-0.4-3-1-Said-1.txt': '53', 'V-SR-50-0.2-3-1-Said-10.txt': '32', 'V-SR-50-0.8-3-1-Said-5.txt': '55', 'V-SR-50-0.6-3-1-Said-7.txt': '46', 'V-SR-50-0.6-3-1-Said-4.txt': '50', 'V-SR-50-0.8-3-1-Said-6.txt': '64', 'V-SR-50-0.6-3-1-Said-3.txt': '47', 'V-SR-50-0.4-3-1-Said-8.txt': '56', 'V-SR-50-0.4-3-1-Said-10.txt': '47', 'V-SR-50-0.6-3-1-Said-8.txt': '80', 'V-SR-50-0.4-3-1-Said-5.txt': '74', 'V-SR-50-0.4-3-1-Said-6.txt': '56', 'V-SR-50-0.8-3-1-Said-8.txt': '85', 'V-SR-50-0.8-3-1-Said-9.txt': '54'}

# Seed used by Bart to generate scenarios for particular instances.
switchSeed50 = {'V-50-0.2-Said-3.txt': 2960798552, 'V-50-0.2-Said-10.txt': 384308366, 'V-50-0.4-Said-8.txt': 3509745835, 'V-50-0.4-Said-10.txt': 4245682032, 'V-50-0.4-Said-3.txt': 3542931274, 'V-50-0.4-Said-1.txt': 1657584292, 'V-50-0.2-Said-4.txt': 939023601, 'V-50-0.6-Said-1.txt': 3172209514, 'V-50-0.8-Said-6.txt': 1312117401, 'V-50-0.2-Said-1.txt': 169746606, 'V-50-0.4-Said-9.txt': 1251246364, 'V-50-0.4-Said-2.txt': 1503575049, 'V-50-0.8-Said-4.txt': 4079022435, 'V-50-0.4-Said-5.txt': 1401351728, 'V-50-0.8-Said-5.txt': 1466238004, 'V-50-0.6-Said-8.txt': 1153046177, 'V-50-0.2-Said-2.txt': 2244339159, 'V-50-0.6-Said-6.txt': 2792356367, 'V-50-0.6-Said-3.txt': 1641050052, 'V-50-0.2-Said-7.txt': 1068872012, 'V-50-0.6-Said-2.txt': 3899387827, 'V-50-0.4-Said-7.txt': 3356153606, 'V-50-0.6-Said-10.txt': 3978020146, 'V-50-0.6-Said-7.txt': 1884145616, 'V-50-0.4-Said-6.txt': 420579141, 'V-50-0.8-Said-3.txt': 3615730262, 'V-50-0.8-Said-2.txt': 229232853, 'V-50-0.2-Said-6.txt': 3730075547, 'V-50-0.2-Said-5.txt': 3026659730, 'V-50-0.2-Said-8.txt': 238168373, 'V-50-0.6-Said-5.txt': 3388060966, 'V-50-0.8-Said-9.txt': 2951863032, 'V-50-0.6-Said-9.txt': 3240941442, 'V-50-0.8-Said-10.txt': 138422084, 'V-50-0.4-Said-4.txt': 2358115439, 'V-50-0.8-Said-8.txt': 2235403639, 'V-50-0.2-Said-9.txt': 3173890614, 'V-50-0.6-Said-4.txt': 1304421, 'V-50-0.8-Said-7.txt': 3399878810, 'V-50-0.8-Said-1.txt': 1660928384}

# Max number of tests for instances with 25 vertices.
maxTests25 = {'V-SR-25-0.6-3-1-Said-2.txt': '27', 'V-SR-25-0.8-3-1-Said-4.txt': '28', 'V-SR-25-0.6-3-1-Said-9.txt': '25', 'V-SR-25-0.8-3-1-Said-10.txt': '32', 'V-SR-25-0.4-3-1-Said-8.txt': '23', 'V-SR-25-0.8-3-1-Said-1.txt': '19', 'V-SR-25-0.4-3-1-Said-9.txt': '26', 'V-SR-25-0.4-3-1-Said-5.txt': '24', 'V-SR-25-0.8-3-1-Said-9.txt': '29', 'V-SR-25-0.4-3-1-Said-7.txt': '8', 'V-SR-25-0.2-3-1-Said-9.txt': '23', 'V-SR-25-0.4-3-1-Said-4.txt': '29', 'V-SR-25-0.8-3-1-Said-7.txt': '14', 'V-SR-25-0.4-3-1-Said-1.txt': '17', 'V-SR-25-0.6-3-1-Said-1.txt': '28', 'V-SR-25-0.2-3-1-Said-4.txt': '14', 'V-SR-25-0.8-3-1-Said-3.txt': '24', 'V-SR-25-0.2-3-1-Said-7.txt': '31', 'V-SR-25-0.8-3-1-Said-5.txt': '20', 'V-SR-25-0.8-3-1-Said-6.txt': '24', 'V-SR-25-0.4-3-1-Said-6.txt': '20', 'V-SR-25-0.6-3-1-Said-7.txt': '16', 'V-SR-25-0.4-3-1-Said-3.txt': '29', 'V-SR-25-0.2-3-1-Said-1.txt': '5', 'V-SR-25-0.8-3-1-Said-8.txt': '10', 'V-SR-25-0.2-3-1-Said-2.txt': '19', 'V-SR-25-0.4-3-1-Said-10.txt': '26', 'V-SR-25-0.6-3-1-Said-10.txt': '24', 'V-SR-25-0.2-3-1-Said-8.txt': '19', 'V-SR-25-0.2-3-1-Said-5.txt': '19', 'V-SR-25-0.6-3-1-Said-8.txt': '34', 'V-SR-25-0.6-3-1-Said-5.txt': '18', 'V-SR-25-0.2-3-1-Said-10.txt': '11', 'V-SR-25-0.4-3-1-Said-2.txt': '23', 'V-SR-25-0.6-3-1-Said-6.txt': '24', 'V-SR-25-0.6-3-1-Said-4.txt': '11', 'V-SR-25-0.2-3-1-Said-3.txt': '20', 'V-SR-25-0.8-3-1-Said-2.txt': '29', 'V-SR-25-0.2-3-1-Said-6.txt': '30', 'V-SR-25-0.6-3-1-Said-3.txt': '22'}


# Switch number used to design formulation and solver type to name the files correctly
switchForm = {1: 'Dickerson', 2: 'EE', 3: 'Cycle'}
switchSolver = {1: 'SR', 2: 'IP', 3:'LP', 4: 'B', 5: 'UC', 6:'EXP', 7: 'MH'}

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
    global cycleLength, chainLength, failureType, nrCpuCluster, testType, postEx, warmStart, benderVersion
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
    parser.add_argument('--testtype', '-ttype', action='store', type=int, default = 1, help="Test type for ex post evaluation: 1 = exact, 2 = monte-carlo. Default = 1")
    parser.add_argument('--postex', '-pe', action='store', type=int, default = 0, help="Replace solver by post-ex evaluation. Default = 0")
    parser.add_argument('--warmstart', '-ws', action='store', type=int, default = 0, help="Warm start option with IP-D ONLY: 0 = no warm start, 1 = warm start with SR arcs, 2 = warm start with rounding LP. Default = 0")
    parser.add_argument('--benderversion', '-bv', action='store', type=int, default = 0, help="Activate constraints on testvar for bender decompositon. 0 = No constraints, 1 = Connecting Constraints, 2 = Cycle constraints. Default = 0")
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
    postEx = args.postex
    warmStart = args.warmstart
    benderVersion = args.benderversion
    if postEx == 1: nrCpuCluster = 1 # If exact expected transplant computation only one CPU is needed


# Return the instance number and probability associated with an instance file "V-50...-Said-10.txt"
def getProbNumber(instanceName):
    instance = instanceName.split(".txt")[0]
    instanceNumber = instance.split("-")[-1]
    instanceProb = instance.split("-")[2]
    return instanceProb, instanceNumber

def getNrVertices(instanceName):
    return instanceName.split("-")[1]

# Convert the name of an instance file to the name of a SR solution file to gather the
# maximum number of tests. It uses the instance number Said-x and the probability of success
def nrMaxTests(instanceName):
    instanceProb, instanceNumber = getProbNumber(instanceName)
    nrVertices = getNrVertices(instanceName)
    SRName = "V-SR-" + nrVertices + "-" + instanceProb + "-3-1-Said-" + instanceNumber + ".txt"
    if nrVertices == "25": return maxTests25[SRName]
    else: return maxTests50[SRName]

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
    nrVertices = getNrVertices(instanceName)
    instanceProb, instanceNumber = getProbNumber(instanceName)
    return "V-" + switchSolver[solverType] + "-" + benderIndicator() + switchForm[formulation] \
    + "-" + nrVertices + "-" + instanceProb + "-3-1-" + str(nrScenarios) + "-Said-" + str(instanceNumber) + ".txt"

# Used only to compute exact expected transplants given a solution file (post-ex evaluation)
# we need, given a solution file, to find the instance file it has been computed from.
def identifyInstance(solutionFileName):
    instanceNb = solutionFileName[-5]
    if (instanceNb == '0'): instanceNb = "10" # Instance number goes from 1 to 10
    pr = solutionFileName.split("-")[-6]
    return "V-50-" + pr + "-Said-" + instanceNb + ".txt"

# Used to indicates which type of constraints are used with Bender's decomposition
def benderIndicator():
    if benderVersion == 1: return "Con-"
    if benderVersion == 2: return "Cyc-"
    return ""


# Creates the config file to launch instanceName. Assumes that the
# directory to store config files exists.
def createConfigFile(instanceName):
    configFileName = "Config-" + instanceName[:-4] + "-" + switchSolver[solverType][0] + "-" + benderIndicator() \
    + switchForm[formulation][0] + "-" + str(nrScenarios) + ".txt"

    configFile = open(configPath + sep + configFileName , "w")
    configFile.truncate(0) # Clean the files if it already exists
    instanceProb, instanceNumber = getProbNumber(instanceName)
    SRName = "V-SR-" + getNrVertices(instanceName) + "-" + instanceProb + "-3-1-Said-" + instanceNumber + ".txt"

    configFile.write("Cyclelength = " + str(cycleLength) + "\n")
    configFile.write("Chainlength = " + str(chainLength) + "\n")
    configFile.write("Failure Type = " + str(failureType) + "\n")
    if postEx == 0: configFile.write("Solver = " + str(solverType) + "\n")
    else: configFile.write("Solver = 6\n")
    configFile.write("Formulation = " + str(formulation) + "\n")
    configFile.write("Bender Version = " + str(benderVersion) + "\n")
    configFile.write("Expected Type = " + str(testType) + "\n")
    configFile.write("Max Test = " + nrMaxTests(instanceName) + "\n")
    if (postEx == 0): configFile.write("Scenarios = " + str(nrScenarios) + "\n")
    else: configFile.write("Scenarios = 10000\n")
    configFile.write("Subset Recourse Size = 1\n")
    configFile.write("Input Type = 2\n")
    configFile.write("Input File = " + instanceName + "\n")
    if warmStart > 0: configFile.write("Seed = " + str(switchSeed[instanceName]) + "\n")
    else: configFile.write("Seed = 0\n")
    configFile.write("Testvar Output = " + getOutputName(instanceName) + "\n")
    configFile.write("Solution Input File = " + getOutputName(instanceName) + "\n")
    configFile.write("Time Limit = " + str(timeLimitCplex) + "\n")
    configFile.write("Memory Limit = " + str(memLimitCplex) + "\n")
    configFile.write("# Limited World = 1")
    configFile.close()
    return configFileName

def createSubmitFile(instanceName, configFileName):
    instanceProb, instanceNumber = getProbNumber(instanceName)
    submitFileName = "Submit-" + instanceName[:-4] + "-" + switchSolver[solverType][0] + "-" + benderIndicator()\
    + switchForm[formulation][0] + "-" + str(nrScenarios)  + ".sh"

    submitFile = open(submitPath + sep + submitFileName, "w")
    submitFile.truncate(0)
    submitFile.write("#!/bin/bash\n\n")
    prefixJn = ""
    if warmStart > 0: prefixJn = "W"
    submitFile.write("#SBATCH --job-name=" + prefixJn + str(instanceNumber) + "-" + str(instanceProb) + switchSolver[solverType][0] + switchForm[formulation][0] + "\n")
    submitFile.write("#SBATCH --time=" + timeLimitCluster + "\n")
    submitFile.write("#\n")
    prefix = "Out-"
    if postEx == 1: prefix = "ExPost-"
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
    if postEx == 0:
        instanceList = glob.glob(dirPath + "/V-50-*.txt")
        if not instanceList: instanceList = glob.glob(dirPath + "/V-25-*.txt")
        for instance in instanceList:
            instanceName = instance.split(sep)[-1]
            configFileName = createConfigFile(instanceName)
            createSubmitFile(instanceName, configFileName)
    if postEx == 1:
        instanceList = glob.glob(dirPath + "/V-B*.txt")
        if not instanceList: instanceList = glob.glob(dirPath + "/V-E*.txt")
        if not instanceList: instanceList = glob.glob(dirPath + "/V-IP*.txt")
        if not instanceList: instanceList = glob.glob(dirPath + "/V-LP*.txt")
        for solutionFile in instanceList:
            solutionFileName = solutionFile.split(sep)[-1]
            instanceFileName = identifyInstance(solutionFileName)
            configFileName = createConfigFile(instanceFileName)
            createSubmitFile(instanceFileName, configFileName)
