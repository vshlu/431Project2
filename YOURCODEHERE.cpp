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
unsigned int currentlyExploringDim = 0;
bool currentDimDone = false;
bool isDSEComplete = false;

/*
 * Given a half-baked configuration containing cache properties, generate
 * latency parameters in configuration string. You will need information about
 * how different cache paramters affect access latency.
 * 
 * Returns a string similar to "1 1 1"
 */
std::string generateCacheLatencyParams(string halfBackedConfig) {

	char latencySettings[3];

	//First retrieve the size of the caches L1D, L1I, and L2
	int l1DSize;
    int l1ISize;
    int l2Size;
    //Cache size = ((block size * 8) * number of sets) / 1024 (to get KB)
    l1DSize = ((extractConfigPararm(halfBackedConfig, 2) * 8) * extractConfigPararm(halfBackedConfig, 3) / 1024);
    l1ISize = ((extractConfigPararm(halfBackedConfig, 2) * 8) * extractConfigPararm(halfBackedConfig, 5) / 1024);
    l2Size = ((extractConfigPararm(halfBackedConfig, 8) * 8) * extractConfigPararm(halfBackedConfig, 7) / 1024);

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
            printf("ERROR IN FINDING MATCH FOR L1D CACHE SIZE");
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
            printf("ERROR IN FINDING MATCH FOR L1I CACHE SIZE");
    }

    //Match size to latency for L1I cache
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
            printf("ERROR IN FINDING MATCH FOR L2 CACHE SIZE");
    }

    //Check associativity of caches to see if latency needs to be modified
    //Check L1D associativity
    switch (extractConfigPararm(halfBackedConfig, 4)){
        case 2:
            l1Dlat += 1;
        case 4:
            l1Dlat += 2;
        default:
            //directly mapped
            l1Dlat = l1Dlat;
    }
    //Check L1I associativity
    switch (extractConfigPararm(halfBackedConfig, 6)){
        case 2:
            l1Ilat += 1;
        case 4:
            l1Ilat += 2;
        default:
            //directly mapped
            l1Ilat = l1Ilat;
    }
    //Check L2 associativity
    switch (extractConfigPararm(halfBackedConfig, 9)){
        case 2:
            l2lat += 1;
        case 4:
            l2lat += 2;
        case 8:
            l2lat += 3;
        case 16:
            l2lat += 4;
        default:
            //directly mapped
            l2lat = l2lat;
    }

    //Calculate the cell to pass for latency settings
    int l1DCell = l1Dlat - 1;
    int l1ICell = l1Ilat - 1;
    int l2Cell = l2lat - 5;

    //Pass calculated cells into collective string
	latencySettings[0] = (char)l1DCell;
    latencySettings[1] = (char)l1ICell;
    latencySettings[2] = (char)l2Cell;
	//
	//YOUR CODE ENDS HERE
	//

	return latencySettings;
}

/*
 * Returns 1 if configuration is valid, else 0
 */
int validateConfiguration(std::string configuration) {
	// The below is a necessary, but insufficient condition for validating a
	// configuration.
	if (isNumDimConfiguration(configuration)){
	    //Make sure L1 instruction cache block size matches the instruction fetch queue size
	    if((extractConfigPararm(configuration, 2) / 8) == (extractConfigPararm(configuration, 0))){
	        //Make sure L2 cache block size is at lease twice L1D (and L1I) cache size
	        if(extractConfigPararm(configuration, 8) >= 2 * ((extractConfigPararm(configuration, 2)))){
	            //Make sure cache sizes are correct
	            int flag = 0;
	            //L1I and L1D cache must be between 2KB and 64KB
                int l1DSize = ((extractConfigPararm(configuration, 2) * 8) * extractConfigPararm(configuration, 3) / 1024);
                int l1ISize = ((extractConfigPararm(configuration, 2) * 8) * extractConfigPararm(configuration, 5) / 1024);
                //L2 cache must be between 32KB and 1024KB
                int l2Size = ((extractConfigPararm(configuration, 8) * 8) * extractConfigPararm(configuration, 7) / 1024);
                if(l1DSize >= 2 && l1DSize <= 64 && l1ISize >= 2 && l1ISize <= 64 && l2Size >= 64 && l2Size <= 1024){
                    return 1;
                }
                else{
                    return 0;
                }
	        }
	        else{
                return 0;
	        }
	    }
	    else{
            return 0;
	    }
	}
	else{
	    return 0;
	}
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
	// Check if proposed configuration has been seen before.
	while (GLOB_seen_configurations[nextconfiguration]) {

		// Check if DSE has been completed before and return current
		// configuration.
		if(isDSEComplete) {
			return currentconfiguration;
		}

		std::stringstream ss;

		string bestConfig;
		if (optimizeforEXEC == 1)
			bestConfig = bestEXECconfiguration;

		if (optimizeforEDP == 1)
			bestConfig = bestEDPconfiguration;

		// Fill in the dimensions already-scanned with the already-selected best
		// value.
		for (int dim = 0; dim < currentlyExploringDim; ++dim) {
			ss << extractConfigPararm(bestConfig, dim) << " ";
		}

		// Handling for currently exploring dimension. This is a very dumb
		// implementation.
		int nextValue = extractConfigPararm(nextconfiguration,
				currentlyExploringDim) + 1;

		if (nextValue >= GLOB_dimensioncardinality[currentlyExploringDim]) {
			nextValue = GLOB_dimensioncardinality[currentlyExploringDim] - 1;
			currentDimDone = true;
		}

		ss << nextValue << " ";

		// Fill in remaining independent params with 0.
		for (int dim = (currentlyExploringDim + 1);
				dim < (NUM_DIMS - NUM_DIMS_DEPENDENT); ++dim) {
			ss << "0 ";
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
		if (currentDimDone) {
			currentlyExploringDim++;
			currentDimDone = false;
		}

		// Signal that DSE is complete after this configuration.
		if (currentlyExploringDim == (NUM_DIMS - NUM_DIMS_DEPENDENT))
			isDSEComplete = true;

		// Keep the following code in this function as-is.
		if (!validateConfiguration(nextconfiguration)) {
			cerr << "Exiting with error; Configuration Proposal invalid: "
					<< nextconfiguration << endl;
			exit(-1);
		}
	}
	return nextconfiguration;
}

