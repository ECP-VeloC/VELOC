/**
 *  @file   topo.c
 *  @author Leonardo A. Bautista Gomez (leobago@gmail.com)
 *  @date   July, 2013
 *  @brief  Topology functions for the VELOC_Mem library.
 */

#include "interface.h"

/*-------------------------------------------------------------------------*/
/**
    @brief      It writes the topology in a file for recovery.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Topo        Topology metadata.
    @param      nameList        The list of the node names.
    @return     integer         VELOC_Mem_SCES if successful.

    This function writes the topology of the system (List of nodes and their
    ID) in a topology file that will be read during recovery to detect which
    nodes (and therefore checkpoit files) are missing in the new topology.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_SaveTopo(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_topology* VELOC_Mem_Topo, char* nameList)
{
    char mfn[VELOC_Mem_BUFS], str[VELOC_Mem_BUFS];
    dictionary* ini;
    int i;

    sprintf(str, "Trying to load configuration file (%s) to create topology.", VELOC_Mem_Conf->cfgFile);
    VELOC_Mem_Print(str, VELOC_Mem_DBUG);

    ini = iniparser_load(VELOC_Mem_Conf->cfgFile);
    if (ini == NULL) {
        VELOC_Mem_Print("Iniparser cannot parse the configuration file.", VELOC_Mem_WARN);
        return VELOC_Mem_NSCS;
    }

    // Set topology section
    iniparser_set(ini, "topology", NULL);

    // Write list of nodes
    for (i = 0; i < VELOC_Mem_Topo->nbNodes; i++) {
        strncpy(mfn, nameList + (i * VELOC_Mem_BUFS), VELOC_Mem_BUFS - 1);
        sprintf(str, "topology:%d", i);
        iniparser_set(ini, str, mfn);
    }

    // Unset sections of the configuration file
    iniparser_unset(ini, "basic");
    iniparser_unset(ini, "restart");
    iniparser_unset(ini, "advanced");

    sprintf(mfn, "%s/Topology.velec_mem", VELOC_Mem_Conf->metadDir);
    sprintf(str, "Creating topology file (%s)...", mfn);
    VELOC_Mem_Print(str, VELOC_Mem_DBUG);

    FILE* fd = fopen(mfn, "w");
    if (fd == NULL) {
        VELOC_Mem_Print("Topology file could NOT be opened", VELOC_Mem_WARN);

        iniparser_freedict(ini);

        return VELOC_Mem_NSCS;
    }

    // Write new topology
    iniparser_dump_ini(ini, fd);

    if (fflush(fd) != 0) {
        VELOC_Mem_Print("Topology file could NOT be flushed.", VELOC_Mem_WARN);

        iniparser_freedict(ini);
        fclose(fd);

        return VELOC_Mem_NSCS;
    }
    if (fclose(fd) != 0) {
        VELOC_Mem_Print("Topology file could NOT be closed.", VELOC_Mem_WARN);

        iniparser_freedict(ini);

        return VELOC_Mem_NSCS;
    }

    iniparser_freedict(ini);

    return VELOC_Mem_SCES;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It reorders the nodes following the previous topology.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Topo        Topology metadata.
    @param      nodeList        The list of the nodes.
    @param      nameList        The list of the node names.
    @return     integer         VELOC_Mem_SCES if successful.

    This function writes the topology of the system (List of nodes and their
    ID) in a topology file that will be read during recovery to detect which
    nodes (and therefore checkpoit files) are missing in the new topology.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_ReorderNodes(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_topology* VELOC_Mem_Topo,
                     int* nodeList, char* nameList)
{
    char mfn[VELOC_Mem_BUFS], str[VELOC_Mem_BUFS], *tmp;
    int i, j, *nl, *old, *new;

    nl = talloc(int, VELOC_Mem_Topo->nbProc);
    old = talloc(int, VELOC_Mem_Topo->nbNodes);
    new = talloc(int, VELOC_Mem_Topo->nbNodes);
    for (i = 0; i < VELOC_Mem_Topo->nbNodes; i++) {
        old[i] = -1;
        new[i] = -1;
    }

    sprintf(mfn, "%s/Topology.velec_mem", VELOC_Mem_Conf->metadDir);
    sprintf(str, "Loading VELOC_Mem topology file (%s) to reorder nodes...", mfn);
    VELOC_Mem_Print(str, VELOC_Mem_DBUG);

    // Checking that the topology file exist
    if (access(mfn, F_OK) != 0) {
        VELOC_Mem_Print("The topology file is NOT accessible.", VELOC_Mem_WARN);

        free(nl);
        free(old);
        free(new);

        return VELOC_Mem_NSCS;
    }

    dictionary* ini;
    ini = iniparser_load(mfn);
    if (ini == NULL) {
        VELOC_Mem_Print("Iniparser could NOT parse the topology file.", VELOC_Mem_WARN);

        free(nl);
        free(old);
        free(new);

        return VELOC_Mem_NSCS;
    }

    // Get the old order of nodes
    for (i = 0; i < VELOC_Mem_Topo->nbNodes; i++) {
        sprintf(str, "Topology:%d", i);
        tmp = iniparser_getstring(ini, str, NULL);
        snprintf(str, VELOC_Mem_BUFS, "%s", tmp);

        // Search for same node in current nameList
        for (j = 0; j < VELOC_Mem_Topo->nbNodes; j++) {
            // If found...
            if (strncmp(str, nameList + (j * VELOC_Mem_BUFS), VELOC_Mem_BUFS) == 0) {
                old[j] = i;
                new[i] = j;
                break;
            } // ...set matching IDs and break out of the searching loop
        }
    }

    iniparser_freedict(ini);

    j = 0;
    // Introducing missing nodes
    for (i = 0; i < VELOC_Mem_Topo->nbNodes; i++) {
        // For each new node..
        if (new[i] == -1) {
            // ..search for an old node not present in the new list..
            while (old[j] != -1) {
                j++;
            }
            // .. and set matching IDs
            old[j] = i;
            new[i] = j;
            j++;
        }
    }
    // Copying nodeList in nl
    for (i = 0; i < VELOC_Mem_Topo->nbProc; i++) {
        nl[i] = nodeList[i];
    }
    // Creating the new nodeList with the old order
    for (i = 0; i < VELOC_Mem_Topo->nbNodes; i++) {
        for (j = 0; j < VELOC_Mem_Topo->nodeSize; j++) {
            nodeList[(i * VELOC_Mem_Topo->nodeSize) + j] = nl[(new[i] * VELOC_Mem_Topo->nodeSize) + j];
        }
    }

    // Free memory
    free(nl);
    free(old);
    free(new);

    return VELOC_Mem_SCES;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It builds the list of nodes in the current execution.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Exec        Execution metadata.
    @param      VELOC_Mem_Topo        Topology metadata.
    @param      nodeList        The list of the nodes to fill.
    @param      nameList        The list of the node names to fill.
    @return     integer         VELOC_Mem_SCES if successful.

    This function makes all the processes to detect in which node are they
    located and distributes the information globally to create an uniform
    mapping structure between processes and nodes.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_BuildNodeList(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                      VELOCT_topology* VELOC_Mem_Topo, int* nodeList, char* nameList)
{
    int i, found, pos, p, nbNodes = 0;
    char hname[VELOC_Mem_BUFS], str[VELOC_Mem_BUFS], *lhn;
    lhn = talloc(char, VELOC_Mem_BUFS* VELOC_Mem_Topo->nbProc);
    memset(lhn + (VELOC_Mem_Topo->myRank * VELOC_Mem_BUFS), 0, VELOC_Mem_BUFS); // To get local hostname
    if (!VELOC_Mem_Conf->test) {
        gethostname(lhn + (VELOC_Mem_Topo->myRank * VELOC_Mem_BUFS), VELOC_Mem_BUFS); // NOT local test
    }
    else {
        snprintf(lhn + (VELOC_Mem_Topo->myRank * VELOC_Mem_BUFS), VELOC_Mem_BUFS, "node%d", VELOC_Mem_Topo->myRank / VELOC_Mem_Topo->nodeSize); // Local
    }
    strncpy(hname, lhn + (VELOC_Mem_Topo->myRank * VELOC_Mem_BUFS), VELOC_Mem_BUFS - 1); // Distributing host names
    MPI_Allgather(hname, VELOC_Mem_BUFS, MPI_CHAR, lhn, VELOC_Mem_BUFS, MPI_CHAR, VELOC_Mem_Exec->globalComm);

    for (i = 0; i < VELOC_Mem_Topo->nbProc; i++) { // Creating the node list: For each process
        found = 0;
        pos = 0;
        strncpy(hname, lhn + (i * VELOC_Mem_BUFS), VELOC_Mem_BUFS - 1); // Get node name of process i
        while ((pos < nbNodes) && (found == 0)) { // Search the node name in the current list of node names
            if (strncmp(&(nameList[pos * VELOC_Mem_BUFS]), hname, VELOC_Mem_BUFS) == 0) { // If we find it break out
                found = 1;
            }
            else { // Else move to the next name in the list
                pos++;
            }
        }
        if (found) { // If we found the node name in the current list...
            p = pos * VELOC_Mem_Topo->nodeSize;
            while (p < pos * VELOC_Mem_Topo->nodeSize + VELOC_Mem_Topo->nodeSize) { // ... we look for empty spot in this node
                if (nodeList[p] == -1) {
                    nodeList[p] = i;
                    break;
                }
                else {
                    p++;
                }
            }
        }
        else { // ... else, we add the new node to the end of the current list of nodes
            strncpy(&(nameList[pos * VELOC_Mem_BUFS]), hname, VELOC_Mem_BUFS - 1);
            nodeList[pos * VELOC_Mem_Topo->nodeSize] = i;
            nbNodes++;
        }
    }
    for (i = 0; i < VELOC_Mem_Topo->nbProc; i++) { // Checking that all nodes have nodeSize processes
        if (nodeList[i] == -1) {
            sprintf(str, "Node %d has no %d processes", i / VELOC_Mem_Topo->nodeSize, VELOC_Mem_Topo->nodeSize);
            VELOC_Mem_Print(str, VELOC_Mem_WARN);
            free(lhn);
            return VELOC_Mem_NSCS;
        }
    }

    free(lhn);

    return VELOC_Mem_SCES;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It builds the list of nodes in the current execution.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Exec        Execution metadata.
    @param      VELOC_Mem_Topo        Topology metadata.
    @param      userProcList    The list of the app. processess.
    @param      distProcList    The list of the distributed processes.
    @param      nodeList        The list of the nodes to fill.
    @return     integer         VELOC_Mem_SCES if successful.

    This function makes all the processes to detect in which node are they
    located and distributes the information globally to create an uniform
    mapping structure between processes and nodes.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_CreateComms(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                    VELOCT_topology* VELOC_Mem_Topo, int* userProcList,
                    int* distProcList, int* nodeList)
{
    MPI_Status status;
    MPI_Group newGroup, origGroup;
    MPI_Comm_group(VELOC_Mem_Exec->globalComm, &origGroup);
    int i, src, buf, group[VELOC_Mem_BUFS]; // VELOC_Mem_BUFS > Max. group size
    if (VELOC_Mem_Topo->amIaHead) {
        MPI_Group_incl(origGroup, VELOC_Mem_Topo->nbNodes * VELOC_Mem_Topo->nbHeads, distProcList, &newGroup);
        MPI_Comm_create(VELOC_Mem_Exec->globalComm, newGroup, &VELOC_Mem_COMM_WORLD);
        for (i = VELOC_Mem_Topo->nbHeads; i < VELOC_Mem_Topo->nodeSize; i++) {
            src = nodeList[(VELOC_Mem_Topo->nodeID * VELOC_Mem_Topo->nodeSize) + i];
            MPI_Recv(&buf, 1, MPI_INT, src, VELOC_Mem_Conf->tag, VELOC_Mem_Exec->globalComm, &status);
            if (buf == src) {
                VELOC_Mem_Topo->body[i - VELOC_Mem_Topo->nbHeads] = src;
            }
        }
    }
    else {
        MPI_Group_incl(origGroup, VELOC_Mem_Topo->nbProc - (VELOC_Mem_Topo->nbNodes * VELOC_Mem_Topo->nbHeads), userProcList, &newGroup);
        MPI_Comm_create(VELOC_Mem_Exec->globalComm, newGroup, &VELOC_Mem_COMM_WORLD);
        if (VELOC_Mem_Topo->nbHeads == 1) {
            MPI_Send(&(VELOC_Mem_Topo->myRank), 1, MPI_INT, VELOC_Mem_Topo->headRank, VELOC_Mem_Conf->tag, VELOC_Mem_Exec->globalComm);
        }
    }
    MPI_Comm_rank(VELOC_Mem_COMM_WORLD, &VELOC_Mem_Topo->splitRank);
    buf = VELOC_Mem_Topo->sectorID * VELOC_Mem_Topo->groupSize;
    for (i = 0; i < VELOC_Mem_Topo->groupSize; i++) { // Group of node-distributed processes (Topology-aware).
        group[i] = distProcList[buf + i];
    }
    MPI_Comm_group(VELOC_Mem_Exec->globalComm, &origGroup);
    MPI_Group_incl(origGroup, VELOC_Mem_Topo->groupSize, group, &newGroup);
    MPI_Comm_create(VELOC_Mem_Exec->globalComm, newGroup, &VELOC_Mem_Exec->groupComm);
    MPI_Group_rank(newGroup, &(VELOC_Mem_Topo->groupRank));
    VELOC_Mem_Topo->right = (VELOC_Mem_Topo->groupRank + 1) % VELOC_Mem_Topo->groupSize;
    VELOC_Mem_Topo->left = (VELOC_Mem_Topo->groupRank + VELOC_Mem_Topo->groupSize - 1) % VELOC_Mem_Topo->groupSize;
    MPI_Group_free(&origGroup);
    MPI_Group_free(&newGroup);
    return VELOC_Mem_SCES;
}

/*-------------------------------------------------------------------------*/
/**
    @brief      It builds and saves the topology of the current execution.
    @param      VELOC_Mem_Conf        Configuration metadata.
    @param      VELOC_Mem_Exec        Execution metadata.
    @param      VELOC_Mem_Topo        Topology metadata.
    @return     integer         VELOC_Mem_SCES if successful.

    This function builds the topology of the system, detects and replaces
    missing nodes in case of recovery and creates the communicators required
    for VELOC_Mem to work. It stores all required information in VELOC_Mem_Topo.

 **/
/*-------------------------------------------------------------------------*/
int VELOC_Mem_Topology(VELOCT_configuration* VELOC_Mem_Conf, VELOCT_execution* VELOC_Mem_Exec,
                 VELOCT_topology* VELOC_Mem_Topo)
{
    int res, c = 0, i, mypos = -1, posInNode;
    char *nameList = talloc(char, VELOC_Mem_Topo->nbNodes *VELOC_Mem_BUFS);

    int* nodeList = talloc(int, VELOC_Mem_Topo->nbNodes* VELOC_Mem_Topo->nodeSize);
    for (i = 0; i < VELOC_Mem_Topo->nbProc; i++) {
        nodeList[i] = -1;
    }

    res = VELOC_Mem_Try(VELOC_Mem_BuildNodeList(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Topo, nodeList, nameList), "create node list.");
    if (res == VELOC_Mem_NSCS) {
        free(nameList);
        free(nodeList);

        return VELOC_Mem_NSCS;
    }

    if (VELOC_Mem_Exec->reco > 0) {
        res = VELOC_Mem_Try(VELOC_Mem_ReorderNodes(VELOC_Mem_Conf, VELOC_Mem_Topo, nodeList, nameList), "reorder nodes.");
        if (res == VELOC_Mem_NSCS) {
            free(nameList);
            free(nodeList);

            return VELOC_Mem_NSCS;
        }
    }

    // Need to synchronize before editing topology file
    MPI_Barrier(VELOC_Mem_Exec->globalComm);
    if (VELOC_Mem_Topo->myRank == 0 && VELOC_Mem_Exec->reco == 0) {
        res = VELOC_Mem_Try(VELOC_Mem_SaveTopo(VELOC_Mem_Conf, VELOC_Mem_Topo, nameList), "save topology.");
        if (res == VELOC_Mem_NSCS) {
            free(nameList);
            free(nodeList);

            return VELOC_Mem_NSCS;
        }
    }

    int *distProcList = talloc(int, VELOC_Mem_Topo->nbNodes);
    int *userProcList = talloc(int, VELOC_Mem_Topo->nbProc - (VELOC_Mem_Topo->nbNodes * VELOC_Mem_Topo->nbHeads));

    for (i = 0; i < VELOC_Mem_Topo->nbProc; i++) {
        if (VELOC_Mem_Topo->myRank == nodeList[i]) {
            mypos = i;
        }
        if ((i % VELOC_Mem_Topo->nodeSize != 0) || (VELOC_Mem_Topo->nbHeads == 0)) {
            userProcList[c] = nodeList[i];
            c++;
        }
    }
    if (mypos == -1) {
        free(userProcList);
        free(distProcList);
        free(nameList);
        free(nodeList);

        return VELOC_Mem_NSCS;
    }

    VELOC_Mem_Topo->nodeRank = mypos % VELOC_Mem_Topo->nodeSize;
    if (VELOC_Mem_Topo->nodeRank == 0 && VELOC_Mem_Topo->nbHeads == 1) {
        VELOC_Mem_Topo->amIaHead = 1;
    }
    else {
        VELOC_Mem_Topo->amIaHead = 0;
    }
    VELOC_Mem_Topo->nodeID = mypos / VELOC_Mem_Topo->nodeSize;
    VELOC_Mem_Topo->headRank = nodeList[(mypos / VELOC_Mem_Topo->nodeSize) * VELOC_Mem_Topo->nodeSize];
    VELOC_Mem_Topo->sectorID = VELOC_Mem_Topo->nodeID / VELOC_Mem_Topo->groupSize;
    posInNode = mypos % VELOC_Mem_Topo->nodeSize;
    VELOC_Mem_Topo->groupID = posInNode;
    for (i = 0; i < VELOC_Mem_Topo->nbNodes; i++) {
        distProcList[i] = nodeList[(VELOC_Mem_Topo->nodeSize * i) + posInNode];
    }

    res = VELOC_Mem_Try(VELOC_Mem_CreateComms(VELOC_Mem_Conf, VELOC_Mem_Exec, VELOC_Mem_Topo, userProcList, distProcList, nodeList), "create communicators.");
    if (res == VELOC_Mem_NSCS) {
        free(userProcList);
        free(distProcList);
        free(nameList);
        free(nodeList);

        return VELOC_Mem_NSCS;
    }

    free(userProcList);
    free(distProcList);
    free(nameList);
    free(nodeList);

    return VELOC_Mem_SCES;
}
