#include <iostream>
#include <stdio.h>
#include <vector>
#include <unordered_map>
#include <bitset>
#include <set>
#include <string.h>
#include <string>
#include <map>
#include <queue>
#include <algorithm>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>


using namespace std;
typedef pair<int, int> PII;
typedef pair<double, int> PDI;
#define MAX_PROPERTY  150

class graph
{
public:
	graph();
	~graph();
	void init();
	void loadGraph(string txt_name, string tag);
	vector<string> split(string textline, string tag);
	int getParent(int son, vector<int> &fa);
	int getParentMap(int son, unordered_map<int, int> &fa);
	void coarsening();
	void unionEdgeForEnum();
	void enumPre(int preID, long long choice, vector<int> &curParent, vector<int> &curSoncnt, vector<int> &curRank, bool *inValid);
	void unionEdgeForGreed();
	void greed1(vector<int> &choice, vector<int> &curParent, vector<int> &curSonCnt, vector<int> &curRank, vector<bool> &invalid);
	int cal(long long cur);
	void compareCrossingEdgeCnt(long long cur);
	void greed2();
	void greed3();
	void unionBlock(vector<int> &choice, int goal);
	void metis(string txt_name, string tag);
	void partition(string txt_name, string tag, string out_file);
	void getFileList(string template_path, string result_path);
	void readQueryResult(string template_path, string result_path, string tag);
	void mergeWCC();
	int getSvCnt();
	// void update();

	string RDF;
	string WEIGHT;

	int part;

private:
	set<string> predicate;
	unordered_map<string, int> entityToID;
	vector<string> IDToEntity;
	unordered_map<string, int> strEntityToID;
	vector<string> IDToStrEntity;
	unordered_map<string, int> predicateToID;
	vector<string> IDToPredicate;
	vector<vector<pair<int, int>>> edge;
	vector<pair<int, string>> otherEdge;

	vector<bitset<MAX_PROPERTY>> containPattern;

	// coarseningPoint[svID]: the disjoint forest of predicate[svID]
	// key: vertex			value: the root of vertex
	vector<unordered_map<int, int>> coarseningPoint;

	// key: entityID		value: the count of triples containing entityID
	vector<int> entityTriples;

	vector<int> WCCParentVec;
	vector<int> WCCSizeVec;
	vector<int> WCCRankVec;
	vector<int> internalPre;
	vector<int> internalAddOrder;

	unordered_map<string, int> edge_cnt; // edge_cnt[IDToPredicate[i]] == edge[i].size()

	// edge_cnt 
	// key: the property	value: the count of the property
	unordered_map<string, int> edge_weight;

	// key: entityID			value: the group ID it belongs to
	unordered_map<string, int> group;
	vector<vector<pair<pair<int, int>, string>>> edgeGroup;

	vector<bool> invalid;
	long long triples;

	// entityCnt : entity count
	long long entityCnt;
	long long strEntityCnt;

	// predicate count
	int preType;
	int svCnt;

	long long limit;
	long long ans;
	long long crossEgdeCnt;
	long long invalidEdgeCnt;

	//read path of query & result
	vector<string> query_template;
	vector<string> query_result;
	unordered_map<string, int> queryID;
	int queryCnt;
	int validResultCnt;

	//read result of query

};