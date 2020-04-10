#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <fstream>
#include <map>
#include <math.h>
#include <fcntl.h>
#include <vector>
#include <iterator>

#include "431project.h"

using namespace std;

/*
 * Enter your PSU ID here to select the appropriate dimension scanning order.
 */
#define MY_PSU_ID 969384744

/*
 * Some global variables to track heuristic progress.
 * 
 * Feel free to create more global variables to track progress of your
 * heuristic.
 */
unsigned int currentlyExploringDim = 12;
bool currentDimDone = false;
bool isDSEComplete = false;

//Gloabals
int l1block[4] = {8,16,32,64};
int ul2block[4] = {16,32,64,128};

int dl1sets[9] = {32,64,128,256,512,1024,2048,4096,8192};
int il1sets[9] = {32,64,128,256,512,1024,2048,4096,8192};
int ul2sets[10] = {256,512,1024,2048,4096,8192,16384,32768,65536,131072};

int dl1assoc[3] = {1,2,4};
int il1assoc[3] = {1,2,4};
int ul2assoc[5] = {1,2,4,8,16};

int width[4] = {1,2,4,8};

/*
 * Given a half-baked configuration containing cache properties, generate
 * latency parameters in configuration string. You will need information about
 * how different cache paramters affect access latency.
 * 
 * Returns a string similar to "1 1 1"
 */
std::string generateCacheLatencyParams(string halfBackedConfig) {
	//string latencySettings;

	//Find block size
	int l1DBlockSize = l1block[extractConfigPararm(halfBackedConfig, 2)];
    int l1IBlockSize = l1block[extractConfigPararm(halfBackedConfig, 2)];
    int l2BlockSize = ul2block[extractConfigPararm(halfBackedConfig, 8)];

    //Find set size
    int l1DSetSize = dl1sets[extractConfigPararm(halfBackedConfig, 3)];
    int l1ISetSize = il1sets[extractConfigPararm(halfBackedConfig, 5)];
    int l2SetSize = ul2sets[extractConfigPararm(halfBackedConfig, 7)];
    //Find associativity
    int l1DAssoc = dl1assoc[extractConfigPararm(halfBackedConfig, 4)];
    int l1IAssoc = il1assoc[extractConfigPararm(halfBackedConfig, 6)];
    int l2Assoc =  ul2assoc[extractConfigPararm(halfBackedConfig, 9)];
	//Calculate the size of the caches L1D, L1I, and L2
    //Cache size = ((block size * 8) * number of sets) / 1024 (to get KB)
    int l1DSize = (((l1DBlockSize) * l1DSetSize) * l1DAssoc) / 1024;
    int l1ISize = (((l1IBlockSize) * l1ISetSize) * l1IAssoc) / 1024;
    int l2Size = (((l2BlockSize) * l2SetSize) * l2Assoc) / 1024;
    //cout << "l1DSize: " <<  l1DSize << " ";
    //cout << "l1ISize: " <<  l1ISize << " ";
    //cout << "l2Size: " <<  l2Size << " ";
    //Match size to latency for L1D cache
    int l1Dlat;
    switch(l1DSize){
        case 2:
            l1Dlat = 1;
            break;
        case 4:
            l1Dlat = 2;
            break;
        case 8:
            l1Dlat = 3;
            break;
        case 16:
            l1Dlat = 4;
            break;
        case 32:
            l1Dlat = 5;
            break;
        case 64:
            l1Dlat = 6;
            break;
        default:
            //Set to largest size
            l1Dlat = 6;
            break;
    }

    //Match size to latency for L1I cache
    int l1Ilat;
    switch(l1ISize){
        case 2:
            l1Ilat = 1;
            break;
        case 4:
            l1Ilat = 2;
            break;
        case 8:
            l1Ilat = 3;
            break;
        case 16:
            l1Ilat = 4;
            break;
        case 32:
            l1Ilat = 5;
            break;
        case 64:
            l1Ilat = 6;
            break;
        default:
            //Set to largest size
            l1Ilat = 6;
            break;
    }

    //Match size to latency for L2 cache
    int l2lat;
    switch(l2Size){
        case 32:
            l2lat = 5;
            break;
        case 64:
            l2lat = 6;
            break;
        case 128:
            l2lat = 7;
            break;
        case 256:
            l2lat = 8;
            break;
        case 512:
            l2lat = 9;
            break;
        case 1024:
            l2lat = 10;
            break;
        default:
            //Set to largest size
            l2lat = 10;
            break;
    }

    //Check associativity of caches to see if latency needs to be modified
    //Check L1D associativity
    switch (l1DAssoc){
        case 2:
            l1Dlat += 1;
            break;
        case 4:
            l1Dlat += 2;
            break;
        default:
            //directly mapped
            l1Dlat = l1Dlat;
            break;
    }

    //Check L1I associativity
    switch (l1IAssoc){
        case 2:
            l1Ilat += 1;
            break;
        case 4:
            l1Ilat += 2;
            break;
        default:
            //directly mapped
            l1Ilat = l1Ilat;
            break;
    }
    //Check L2 associativity
    switch (l2Assoc){
        case 2:
            l2lat += 1;
            break;
        case 4:
            l2lat += 2;
            break;
        case 8:
            l2lat += 3;
            break;
        case 16:
            l2lat += 4;
            break;
        default:
            //directly mapped
            l2lat = l2lat;
            break;
    }

    //Calculate the cell to pass for latency settings
    int l1DCell = l1Dlat - 1;
    int l1ICell = l1Ilat - 1;
    int l2Cell = l2lat - 5;

    //Pass calculated cells into collective string
    stringstream latencySettings;
    latencySettings << " " << l1DCell << " " << l1ICell << " " << l2Cell;

   //cout << "latencySettings: " << latencySettings.str();
	//
	//YOUR CODE ENDS HERE
	//

	return latencySettings.str();
}

/*
 * Returns 1 if configuration is valid, else 0
 */
int validateConfiguration(std::string configuration) {
	// The below is a necessary, but insufficient condition for validating a
	// configuration.
	//Could probably make this block global but
    //Find block size
    int l1DBlockSize = l1block[extractConfigPararm(configuration, 2)];
    int l1IBlockSize = l1block[extractConfigPararm(configuration, 2)];
    int l2BlockSize = ul2block[extractConfigPararm(configuration, 8)];

    //Find set size
    int l1DSetSize = dl1sets[extractConfigPararm(configuration, 3)];
    int l1ISetSize = il1sets[extractConfigPararm(configuration, 5)];
    int l2SetSize = ul2sets[extractConfigPararm(configuration, 7)];
    //Find associativity
    int l1DAssoc = dl1assoc[extractConfigPararm(configuration, 4)];
    int l1IAssoc = il1assoc[extractConfigPararm(configuration, 6)];
    int l2Assoc =  ul2assoc[extractConfigPararm(configuration, 9)];
    //Calculate the size of the caches L1D, L1I, and L2
    //Cache size = ((block size * 8) * number of sets) / 1024 (to get KB)
    int l1DSize = (((l1DBlockSize) * l1DSetSize) * l1DAssoc) / 1024;
    int l1ISize = (((l1IBlockSize) * l1ISetSize) * l1IAssoc) / 1024;
    int l2Size = (((l2BlockSize) * l2SetSize) * l2Assoc) / 1024;

    int fetchQSize = width[extractConfigPararm(configuration, 0)];

    int flag = 1;
    //Configuration is incorrect size
	if(!isNumDimConfiguration(configuration)) {
        flag = 0;
    }
	//Make sure L1 instruction cache block size matches the instruction fetch queue size
	if((l1DBlockSize / 8) == fetchQSize) {
        flag = 1;
    }
	//Make sure L2 cache block size is at lease twice L1D (and L1I) cache size
	if(l2BlockSize >= (2 * l1DBlockSize)) {
        //Make sure cache sizes are correct
        //L1I and L1D cache must be between 2KB and 64KB
        //L2 cache must be between 32KB and 1024KB
        if (l1DSize >= 2 && l1DSize <= 64 && l1ISize >= 2 && l1ISize <= 64 && l2Size >= 64 && l2Size <= 1024) {
            return 1;
        }
        flag = 0;
    }
    return flag;
}

/*
 * Given the current best known configuration, the current configuration,
 * and the globally visible map of all previously investigated configurations,
 * suggest a previously unexplored design point. You will only be allowed to
 * investigate 1000 design points in a particular run, so choose wisely.
 *
 * In the current implementation, we start from the leftmost dimension and
 * explore all possible options for this dimension and then go to the next
 * dimension until the rightmost dimension.
 */
std::string generateNextConfigurationProposal(std::string currentconfiguration,
		std::string bestEXECconfiguration, std::string bestEDPconfiguration,
		int optimizeforEXEC, int optimizeforEDP) {

	//
	// Some interesting variables in 431project.h include:
	//
	// 1. GLOB_dimensioncardinality
	// 2. GLOB_baseline
	// 3. NUM_DIMS
	// 4. NUM_DIMS_DEPENDENT
	// 5. GLOB_seen_configurations

	std::string nextconfiguration = currentconfiguration;
    string bestConfig;
    if (optimizeforEXEC == 1)
        bestConfig = bestEXECconfiguration;

    if (optimizeforEDP == 1)
        bestConfig = bestEDPconfiguration;
	// Check if proposed configuration has been seen before.
    int nextValue = extractConfigPararm(bestConfig, currentlyExploringDim);
	while (!validateConfiguration(nextconfiguration)|| GLOB_seen_configurations[nextconfiguration]) {

		// Check if DSE has been completed before and return current
		// configuration.
		if(isDSEComplete) {
			return currentconfiguration;
		}

		std::stringstream ss;

		if (optimizeforEXEC == 1)
			bestConfig = bestEXECconfiguration;

		if (optimizeforEDP == 1)
			bestConfig = bestEDPconfiguration;

		// Fill in the dimensions already-scanned with the already-selected best
		// value.
		for (int dim = 0; dim < currentlyExploringDim; ++dim) {
			ss << extractConfigPararm(bestConfig, dim) << " ";
		}

		// Handling for currently exploring dimension.
		//Dimensions to be explored:
		//BP
		//Cache
		//Core
		//FPU
		cout<<"currentlyExploringDim -" << currentlyExploringDim;

        switch (currentlyExploringDim){
		    case 12:
		        //there are 5 possible settings for branch predictor
		        if(nextValue >= 0 && nextValue < 4){
		            nextValue = nextValue + 1;
		            cout<<"NEXT VALUE__"<<nextValue;
		            break;
		        }
            case 1:
                //There are only 2 possible settings for the core
                if(nextValue >= 0 && nextValue < 1){
                    nextValue++;
                    break;
                }
            case 0:
                //FPU has 4 possible settings
                if(nextValue >= 0 && nextValue < 3){
                    nextValue++;
                    break;
                }

            //These are the cases for cache
            //There are many different things to change for the cache

            //begin with l1i settings
            case 2:
                //ilblock - 4 settings
                if(nextValue >= 0 && nextValue < 3){
                    nextValue++;
                    break;
                }
            case 5:
                //ilsets - 9 settings
                if(nextValue >= 0 && nextValue < 8){
                    nextValue++;
                    break;
                }

            case 6:
                //ilassoc - 3 settings
                if(nextValue >= 0 && nextValue < 2){
                    nextValue++;
                    break;
                }

            //now l1d settings

            //since l1 block is shared for i and d cache we do not need to reiterate
            case 3:
                //dl sets has 9 settings
                if(nextValue >= 0 && nextValue < 8){
                    nextValue++;
                    break;
                }
            case 4:
                //dl assoc has 3 settings
                if(nextValue >= 0 && nextValue < 2){
                    nextValue++;
                    break;
                }

            //now u2 settings
            case 8:
                //u2 block has 4 settings
                if(nextValue >= 0 && nextValue < 3){
                    nextValue++;
                    break;
                }
            case 7:
                //u2 sets has 10 settings
                if(nextValue >= 0 && nextValue < 9){
                    nextValue++;
                    break;
                }
            case 9:
                //u2 assoc has 5 settings
                if(nextValue >= 0 && nextValue < 4){
                    nextValue++;
                    break;
                }
            default:
                //somehow we are in a setting we dont want so dont change next value and the next cycle will send a repeat config
                nextValue = nextValue;
                break;
		}

		//the if statments in the select will prevent the nextValue from being too big
		if (nextValue == GLOB_dimensioncardinality[currentlyExploringDim]) {
			currentDimDone = true;
		}
		ss << nextValue << " ";
		// Fill in remaining independent params with remaining values from best config.
		for (int dim = (currentlyExploringDim + 1); dim < (NUM_DIMS - NUM_DIMS_DEPENDENT); ++dim) {
		    if((dim + 1) >= (NUM_DIMS - NUM_DIMS_DEPENDENT)){
                ss << extractConfigPararm(bestConfig, dim);
		    }
		    else{
                ss << extractConfigPararm(bestConfig, dim) << " ";
		    }
		}

		//
		// Last NUM_DIMS_DEPENDENT3 configuration parameters are not independent.
		// They depend on one or more parameters already set. Determine the
		// remaining parameters based on already decided independent ones.
		//
		string configSoFar = ss.str();
		// Populate this object using corresponding parameters from config.
		ss << generateCacheLatencyParams(configSoFar);
		// Configuration is ready now.
		nextconfiguration = ss.str();

		// Make sure we start exploring next dimension in next iteration.
		//order of exploration
		//BP
		//Cache
		//Core
		//FPU
		if (currentDimDone) {
		    switch (currentlyExploringDim){
		        case 12:
		            //we have finished exploring the BP settings and are now onto the cache
                    currentlyExploringDim = 2;
                    currentDimDone = false;
                //now we must check all the conditions for the cache
		        case 2:
		            //finished with l1 block, move onto l1 sets
		            currentlyExploringDim = 5;
		            currentDimDone = false;
                case 5:
                    //finished with l1 sets, move onto l1 assoc
                    currentlyExploringDim = 6;
                    currentDimDone = false;

                case 6:
                    //finished with l1 assoc, move onto l1d sets
                    currentlyExploringDim = 3;
                    currentDimDone = false;

                //now l1d settings

                //since l1 block is shared for i and d cache we do not need to reiterate
                case 3:
                    //finished with l1d sets, move onto l1d assoc
                    currentlyExploringDim = 4;
                    currentDimDone = false;
                case 4:
                    //finished with l1d assoc, move onto u2 settings
                    currentlyExploringDim = 8;
                    currentDimDone = false;

                //now u2 settings
                case 8:
                    //finished with u2 blocks, move onto u2 sets
                    currentlyExploringDim = 7;
                    currentDimDone = false;
                case 7:
                    //finished with u2 sets, move onto u2 assoc
                    currentlyExploringDim = 9;
                    currentDimDone = false;
                case 9:
                    //we are finished
                    currentDimDone = false;
		        default:
		            //in this case something is wrong so we will end the current exploration
		            currentDimDone = false;
		    }
		}

		// Signal that DSE is complete after this configuration.
		if (currentlyExploringDim == (NUM_DIMS - NUM_DIMS_DEPENDENT))
			isDSEComplete = true;

		// Keep the following code in this function as-is.
		if (!validateConfiguration(nextconfiguration)) {
			cerr << "Exiting with error; Configuration Proposal invalid: "
					<< nextconfiguration << endl;
			continue;
		}
        //cout<<"HERE";
	}
	return nextconfiguration;
}

