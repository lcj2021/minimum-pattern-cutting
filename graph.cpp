#include "graph.h"
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <time.h>
#include "Utils.h"

graph::graph() {}

graph::~graph() {}

void graph::init()
{
	vector<pair<int, int>> tmp;
	edge.push_back(tmp);

	entityTriples.push_back(0);

	IDToEntity.push_back("");
	IDToPredicate.push_back("");
	preType = entityCnt = triples = invalidEdgeCnt = 0;
}

int graph::getSvCnt()
{
	svCnt = preType + validResultCnt;
	for (int i = 1; i <= preType; ++i)
		// if (edge_weight[IDToPredicate[i]] == 0 || !edge_weight.count(IDToPredicate[i]))
			edge_weight[IDToPredicate[i]] = 1;
	for (int i = preType + 1; i <= svCnt; ++i)
	{
		edge_weight[IDToPredicate[i]] = 10000;
		cout << i << " : " <<  IDToPredicate[i] << ' ' << edge[i].size() << endl;
		// cout << predicateToID[IDToPredicate[i]] << " : " << IDToPredicate[i] << endl;
	}

	return svCnt;
}

void graph::loadGraph(string txt_name, string tag)
{
	string line;
	ifstream in1(txt_name);
//	ofstream out("dbpedia2014_100K.nt");
	vector<pair<int, int>> tmp;
	cout << txt_name << "========" << endl;
	while (getline(in1, line))
	{
		if (triples % 10000 == 0)
			cout << "loading triples : " << triples << endl;
		triples++;
		line.resize(line.length() - 2);
		vector<string> s;
		s = Utils::split(line, tag);

		predicate.insert(s[1]);

		//取a和b,确保在unordered_map中加入映射
		for (int i = 0; i < 3; i += 2)
		{
			//如果s[0] 或 s[2]是 entity 并且 entity - ID 映射中没有
			if ((s[i][0] == '<' || s[i][0] == '_') && entityToID.count(s[i]) == 0)
			{
				entityToID[s[i]] = ++entityCnt;
				IDToEntity.push_back(s[i]);
				entityTriples.push_back(0);
			}
			else if ((s[i][0] == '"') && strEntityToID.count(s[i]) == 0)
			{
				strEntityToID[s[i]] = ++strEntityCnt;
				IDToStrEntity.push_back(s[i]);
				entityTriples.push_back(0);
			}
		}

		edge_cnt[s[1]]++;
		int a = entityToID[s[0]];

		//a实体 对应的三元组数量 ++
		entityTriples[a]++;

		//a 和 b同时为entity 谓词才能算preType
		if ((s[0][0] == '<' || s[0][0] == '_') && (s[2][0] == '<' || s[2][0] == '_'))
		{
			if (predicateToID.count(s[1]) == 0)
			{
				predicateToID[s[1]] = ++preType;
				IDToPredicate.push_back(s[1]);
				edge.push_back(tmp);
			}
			int b = entityToID[s[2]];
			//加到谓词所属的边集
			edge[predicateToID[s[1]]].push_back({a, b});

			//b实体 对应的三元组数量 ++
			entityTriples[b]++;
		}
	}
	in1.close();

	// ifstream in2(weight);
	// string pred, freq_str;
	// int freq;
	// while(getline(in2, line))
	// {
	// 	auto sep = line.find(" ");
	// 	pred = line.substr(0, sep);
	// 	freq_str = line.substr(sep + 1);
	// 	freq = stoi(freq_str);
	// 	cout << pred << " " << freq << endl;
	// 	edge_weight[pred] = (double)freq;
	// }
	// in2.close();

	limit = entityCnt / part / 1;
	printf("limit: %lld\n", limit);
	printf("triples: %lld\n", triples);
	printf("entityCnt: %lld\n", entityCnt);
	printf("strEntityCnt: %lld\n", strEntityCnt);
	printf("predicate: %lu\n", predicate.size());
	printf("entity->preType: %d\n", preType);
	 

	// for (int i = 1; i <= preType; ++ i)
	// 	printf("sizeof edge_cnt[%d] : %ld\n", i, edge_cnt[IDToPredicate[i]]);
	// containPattern = vector<bitset<MAX_PROPERTY>>(entityCnt + 1);
}

void graph::coarsening()
{
	// cout << preType << " ? " << edge.size() - 1 << endl;
	// cout << validResultCnt << " ? " << edgeOfMultiPre.size() - 1<< endl;
	invalid = vector<bool>(svCnt + 1, 0);
	coarseningPoint = vector<unordered_map<int, int>>(svCnt + 1, unordered_map<int, int>());
	for (int svID = 1; svID <= svCnt; ++ svID)
	{
		vector<int> sonCnt = vector<int>(entityCnt + 1, 1);
		vector<int> rank = vector<int>(entityCnt + 1, 0);

		//遍历 标签为 svID 的所有边
		for (int p = 0; p < edge[svID].size(); ++p)
		{
			int A = edge[svID][p].first, B = edge[svID][p].second;

			//孤立点, 以自环形式加入 标签为 svID 的WCC
			if (coarseningPoint[svID].count(A) == 0)
				coarseningPoint[svID].insert(make_pair(A, A));
			if (coarseningPoint[svID].count(B) == 0)
				coarseningPoint[svID].insert(make_pair(B, B));

			//找到 A B 所属 WCC 中 父节点
			int parentA = Utils::Utils::getParentMap(A, coarseningPoint[svID]);
			int parentB = Utils::Utils::getParentMap(B, coarseningPoint[svID]);

			//令A的color点权重 > B
			if (rank[parentA] < rank[parentB])
				swap(parentA, parentB);

			//A B在两个 WCC树 里面
			if (parentA != parentB)
			{
				//把B 归入 A的WCC
				coarseningPoint[svID][parentB] = parentA;
				sonCnt[parentA] += sonCnt[parentB];
				rank[parentA] = max(rank[parentA], rank[parentB] + 1);

				//某个WCC规模超过了limit
				if (sonCnt[parentA] > limit)
				{
					invalid[svID] = 1;
					invalidEdgeCnt++;
					string preName;
					preName = IDToPredicate[svID];

					printf("invalid: %d %s\n", svID, preName.data());
					break;
				}
			}
		}
	}
}

void graph::unionEdgeForEnum()
{
	//缩点
	coarsening();

	//判断当前选择 WCC规模是否合法
	bool *invalidST = new bool[1 << preType]();
	for (int i = 1; i <= preType; ++ i)
		if (invalid[i])
			invalidST[1 << (i - 1)] = 1;

	//ans中 1代表内部属性 0代表交叉边
	ans = 0;
	vector<int> choice(preType + 1, 0);

	vector<int> parent = vector<int>(entityCnt + 1);
	for (int i = 1; i <= entityCnt; ++ i)
		parent[i] = i;
	vector<int> sonCnt = vector<int>(entityCnt + 1, 1);
	vector<int> rank = vector<int>(entityCnt + 1, 0);

	crossEgdeCnt = preType;
	printf("enumPre\n");
	enumPre(0, 0, parent, sonCnt, rank, invalidST);

	printf("crossEgdeCnt: %d\n", preType - Utils::count1InBinary(ans));
	for (int i = 1; i <= preType; ++ i)
	{
		//ans记录当前选择
		choice[i] = ((1 << (i - 1)) & ans) ? 1 : 0;
		// if(choice[i]==0)cout<<i<<"	"<<IDToPredicate[i]<<endl;
	}
	printf("\n");
	unionBlock(choice, part);
	delete[] invalidST;
}

//dfs枚举所有2^preID中选择
void graph::enumPre(int preID, long long choice, vector<int> &curParent, vector<int> &curSonCnt, vector<int> &curRank, bool *invalidST)
{
	//最优剪枝
	if (Utils::count1InBinary(ans) >= Utils::count1InBinary(choice) + preType - preID - invalidEdgeCnt)
		return;

	//边界
	if (preID == preType)
	{
		printf("choice(Binary) when preID == preType: %lld\n", choice);
		compareCrossingEdgeCnt(choice);
		return;
	}
	preID++;

	//选择加上当前 谓词
	long long nextchoice = choice | (1LL << (preID - 1));

	//flag为1: 选择非法
	bool flag = invalidST[nextchoice] | invalidST[1LL << (preID - 1)];

	if (!flag)
	{
		vector<int> nextFa(curParent);
		vector<int> nextSonCnt(curSonCnt);
		vector<int> nextRank(curRank);
		unordered_map<int, int>::iterator it;
		for (it = coarseningPoint[preID].begin(); it != coarseningPoint[preID].end(); ++ it)
		{
			int point = it->first;
			int parentA = Utils::getParent(point, nextFa), parentB = Utils::getParent(Utils::Utils::getParentMap(point, coarseningPoint[preID]), nextFa);
			if (nextRank[parentA] < nextRank[parentB])
				swap(parentA, parentB);
			if (parentA != parentB)
			{
				nextFa[parentB] = parentA;
				nextSonCnt[parentA] += nextSonCnt[parentB];
				nextRank[parentA] = max(nextRank[parentA], nextRank[parentB] + 1);
				if (nextSonCnt[parentA] > limit)
				{
					flag = 1;
					break;
				}
			}
		}

		if (!flag)
			enumPre(preID, nextchoice, nextFa, nextSonCnt, nextRank, invalidST);
	}

	//如果不合法 就跳过当前谓词,选下一个
	if (flag)
	{
		for (int i = 1; i <= preType; ++ i)
			invalidST[(1LL << (i - 1)) | nextchoice] = 1;
	}
	enumPre(preID, choice, curParent, curSonCnt, curRank, invalidST);
}

void graph::unionEdgeForGreed()
{
	coarsening();

	ans = 0;
	vector<int> choice(svCnt + 1, 0);
	vector<int> parent = vector<int>(entityCnt + 1);
	for (int i = 1; i <= entityCnt; ++ i)
		parent[i] = i;
	vector<int> sonCnt = vector<int>(entityCnt + 1, 1);
	vector<int> rank = vector<int>(entityCnt + 1, 1);
	printf("greed1\n");

	int threshold = entityCnt * 0.0001;

	int optim = 0;

	for (int preID = 1; preID <= svCnt; ++ preID)
	{
		if (edge[preID].size() < threshold)
		{
			for (int p = 0; p < edge[preID].size(); ++ p)
			{

				int A = edge[preID][p].first, B = edge[preID][p].second;
				int parentA = Utils::getParent(A, parent), parentB = Utils::getParent(B, parent);

				if (rank[parentA] < rank[parentB])
					swap(parentA, parentB);
				if (parentA != parentB)
				{
					parent[parentB] = parentA;
					sonCnt[parentA] += sonCnt[parentB];
					rank[parentA] = max(rank[parentA], rank[parentB] + 1);
				}
			}
			choice[preID] = 1;
			optim++;
		}
	}

	printf("opt: %d\n", optim);

	greed1(choice, parent, sonCnt, rank, invalid);
	// WCCParentVec = parent;
	internalPre = choice;
	// WCCSizeVec = sonCnt;

	int crossEdge = 0;
	for (int preID = 1; preID <= svCnt; ++ preID)
		if (internalPre[preID] == 0)
			cout << preID << " " << IDToPredicate[preID] << endl, crossEdge++;
	printf("crossEdge: %d\n", crossEdge);
	printf("\n");
	unionBlock(internalPre, part);
}

void graph::greed1(vector<int> &choice, vector<int> &curParent, vector<int> &curSonCnt, vector<int> &curRank, vector<bool> &invalid)
{
	//越小越好
	int nextMinCost = 0x3f3f3f3f;

	if (true)
	{
		vector<int> nextBestParent(curParent);
		vector<int> nextBestSonCnt(curSonCnt);
		vector<int> nextBestRank(curRank);
		vector<int> nextBestChoice(choice);
		for (int preID = 1; preID <= svCnt; ++ preID)
		{
			//如果 当前谓词作为交叉边 且 当前谓词有效
			if (choice[preID] == 0 && !invalid[preID])
			{
				vector<int> nextParent(curParent);
				vector<int> nextSonCnt(curSonCnt);
				vector<int> nextRank(curRank);
				int nextBlockNum = 0;
				bool flag = 0;
				int curMax = -1;

				//同一谓词森林 下 树之间的合并
				for (const auto &it : coarseningPoint[preID])
				{
					int point = it.first;
					int parentA = Utils::getParent(point, nextParent), parentB = Utils::getParent(Utils::Utils::getParentMap(point, coarseningPoint[preID]), nextParent);
					if (nextRank[parentA] < nextRank[parentB])
						swap(parentA, parentB);
					if (parentA != parentB)
					{
						nextParent[parentB] = parentA;
						nextSonCnt[parentA] += nextSonCnt[parentB];
						nextRank[parentA] = max(nextRank[parentA], nextRank[parentB] + 1);
						if (nextSonCnt[parentA] > limit)
						{
							flag = 1;
							break;
						}
						curMax = max(nextSonCnt[parentA], curMax);
					}
				}

				if (flag)
					continue;

				// double curCost = (double)curMax / edge_weight[IDToPredicate[preID]];
				double curCost = curMax / edge_weight[IDToPredicate[preID]];
				// cout << "cost of ID:" << preID << " " << curCost << endl;
				// cout << preID << " " << IDToPredicate[preID] << " " << edge_weight[IDToPredicate[preID]] << endl;

				//全部森林中 树的数量
				for (int p = 1; p <= entityCnt; ++ p)
					if (Utils::getParent(p, nextParent) == p)
						nextBlockNum++;

				if (!nextMinCost || curCost < nextMinCost)
				{
					nextMinCost = curCost;
					nextBestParent.assign(nextParent.begin(), nextParent.end());
					nextBestSonCnt.assign(nextSonCnt.begin(), nextSonCnt.end());
					nextBestRank.assign(nextRank.begin(), nextRank.end());
					nextBestChoice.assign(choice.begin(), choice.end());
					nextBestChoice[preID] = 1;
					// cout << preID << endl;
				}
			}
		}


		choice.assign(nextBestChoice.begin(), nextBestChoice.end());
		curParent.assign(nextBestParent.begin(), nextBestParent.end());
		curSonCnt.assign(nextBestSonCnt.begin(), nextBestSonCnt.end());
		curRank.assign(nextBestRank.begin(), nextBestRank.end());
		// for(int i = 1; i <= preType; ++ i)
		// 	cout << "choice of " << i << "is " << choice[i] << endl;
		// cout << endl;
	}

	if (nextMinCost != 0x3f3f3f3f)
	{
		greed1(choice, curParent, curSonCnt, curRank, invalid);
	}
	
}

void graph::mergeWCC()
{
	ofstream out("./testOnContainPattern.txt");
	long long blockSum = 0;
	unordered_set<int> pendingWCCOfPattern;
	bitset<MAX_PROPERTY> validPatternMask;
	validPatternMask.set();

	for (int i = preType + 1; i <= svCnt; ++ i)
	{
		if (!internalPre[i])	
			validPatternMask[i - preType] = 0;
	}
	// cout << validPatternMask << endl;

	for (int i = 1; i <= entityCnt; ++ i)
	{
		containPattern[i] &= validPatternMask;
		out << containPattern[i] << endl;
	}

	int patternVertexCnt = 0;
	for (int i = 1; i <= entityCnt; ++ i)
	{
		int p = Utils::getParent(i, WCCParentVec);
		if (containPattern[p].count() != 0)
		{
			// cout << i << endl;
			++ patternVertexCnt;
			containPattern[p] = containPattern[i] | containPattern[p];
			pendingWCCOfPattern.insert(p);
		}
	}
	cout << patternVertexCnt << endl;

	while (pendingWCCOfPattern.size())
	{
		puts("=====================================");
		// compare key: contain pattern cnt, WCC size
		int selectedRootWCC = *pendingWCCOfPattern.begin();
		PII selectedRootPII = {containPattern[selectedRootWCC].count(), WCCSizeVec[selectedRootWCC]};
		for (auto it : pendingWCCOfPattern)
		{
			PII currPII = {containPattern[it].count(), WCCSizeVec[it]};
			if (currPII < selectedRootPII)
				selectedRootPII = currPII, selectedRootWCC = it;
		}
		// cout << selectedWCC << endl;
		cout << selectedRootPII.first << ' ' << selectedRootPII.second << endl;
		pendingWCCOfPattern.erase(selectedRootWCC);

		// if (!pendingWCCOfPattern.size())	break;

		while (WCCSizeVec[selectedRootWCC] < limit && pendingWCCOfPattern.size())
		{
			// compare key: similarity, size difference
			int candidateWCC = *pendingWCCOfPattern.begin();
			PDI candidatePDI = {-(double)(containPattern[candidateWCC] & containPattern[selectedRootWCC]).count() 
				/ (containPattern[candidateWCC] | containPattern[selectedRootWCC]).count(), 
				abs(WCCSizeVec[candidateWCC] - WCCSizeVec[selectedRootWCC])};
			for (auto it : pendingWCCOfPattern)
			{
				double similarity = (double)(containPattern[it] & containPattern[selectedRootWCC]).count() 
				/ (containPattern[it] | containPattern[selectedRootWCC]).count();
				// cout << containPattern[it] << endl << containPattern[selectedRootWCC] << " : " << similarity << endl;
				PDI currPDI = {-similarity, abs(WCCSizeVec[it] - WCCSizeVec[selectedRootWCC])};
				// cout << currPDI.first << ' ' << currPDI.second << endl;
				if (currPDI < candidatePDI)		
					candidatePDI = currPDI, candidateWCC = it;
			}
			// cout << - candidatePDI.first << ' ' << candidatePDI.second << endl;
			cout << candidateWCC << ' ' << WCCSizeVec[candidateWCC] << endl;

			if (WCCSizeVec[selectedRootWCC] + WCCSizeVec[candidateWCC] > limit)		break;
			pendingWCCOfPattern.erase(candidateWCC);
			WCCSizeVec[selectedRootWCC] += WCCSizeVec[candidateWCC];
			WCCParentVec[candidateWCC] = WCCParentVec[selectedRootWCC];
			entityTriples[selectedRootWCC] += entityTriples[candidateWCC];
		}
		cout << "final size of " << selectedRootWCC << " is " << WCCSizeVec[selectedRootWCC] << endl;
		puts("=====================================");
	}
	cout << "limit : " << limit << endl;
}

//最大化内部属性的个数,判断当前选择的内部属性个数是否>现有情况
void graph::compareCrossingEdgeCnt(long long cur)
{
	//c cur中1的个数,即选择的内部属性的个数  cnt cur中0对应的 pre标签的边集规模的和
	int c = 0, cnt = 0;
	for (int i = 0; i < preType; ++ i)
	{
		if (cur & (1LL << i))
			c++;
		else
			cnt += edge[i + 1].size();
	}
	if (Utils::count1InBinary(ans) < c)
		ans = cur, crossEgdeCnt = cnt;
	else if (Utils::count1InBinary(ans) == c && crossEgdeCnt > cnt)
		ans = cur, crossEgdeCnt = cnt;
}

void graph::greed2()
{
	printf("greed2\n");
	invalid = vector<bool>(svCnt + 1, 0);
	int threshold = entityCnt * 0.0001;
	vector<int> fa(entityCnt + 1);
	vector<int> FA(entityCnt + 1);
	for (int i = 1; i <= entityCnt; ++ i)
		fa[i] = FA[i] = i;
	vector<int> RANK(entityCnt + 1, 0);

	//SONCNT record the size of each weakly connected component in final result
	vector<int> SONCNT(entityCnt + 1, 1);

	//choice is used to determine if a property is selected as internal, 1 means internal
	vector<int> choice(svCnt + 1, 0);
	vector<pair<int, int>> arr;

	for (int preID = 1; preID <= svCnt; ++ preID)
	{
		//如果preID对应的边集规模小于门槛
		if (edge_cnt[IDToPredicate[preID]] < threshold)
		{
			//枚举preID对应边集的所有边
			for (int p = 0; p < edge[preID].size(); ++ p)
			{

				int A = edge[preID][p].first, B = edge[preID][p].second;
				int parentA = Utils::getParent(A, FA), parentB = Utils::getParent(B, FA);

				//令A对应color的权重 > B
				if (RANK[parentA] < RANK[parentB])
					swap(parentA, parentB);
				if (parentA != parentB)
				{
					FA[parentB] = parentA;
					SONCNT[parentA] += SONCNT[parentB];
					RANK[parentA] = max(RANK[parentA], RANK[parentB] + 1);
				}
			}

			//把preID这个作为内部属性
			choice[preID] = 1;
		}
		//如果preID对应的边集规模 >= 门槛
		else
		{
			vector<int> parent(fa);
			vector<int> sonCnt = vector<int>(entityCnt + 1, 1);

			//int MaxCntSize = 0;
			for (int p = 0; p < edge[preID].size(); ++ p)
			{

				int A = edge[preID][p].first, B = edge[preID][p].second;
				int parentA = Utils::getParent(A, parent), parentB = Utils::getParent(B, parent);

				if (parentA != parentB)
				{
					parent[parentB] = parentA;
					//sonCnt record the size of each weakly connected component in intermediate result
					sonCnt[parentA] += sonCnt[parentB];
					//if(sonCnt[parentA]>MaxCntSize){
					//	MaxCntSize = sonCnt[parentA];
					//}
					if (sonCnt[parentA] > limit)
					{
						invalid[preID] = 1;
						printf("invalid: %d\n", preID);
						break;
					}
				}
			}
			if (invalid[preID])
				continue;

			//sorting properties by the numbers of weakly connected components
			int SonCntNum = 0;

			//确定preID森林中 树根的数量
			for (int p = 1; p <= entityCnt; ++ p)
				if (Utils::getParent(p, parent) == p)
					SonCntNum++;
			arr.push_back(make_pair(SonCntNum, preID));

			//arr.push_back(make_pair(MaxCntSize,preID));
		}
	}

	sort(arr.begin(), arr.end());
	/*put as many properties as possbile into the internal properties*/
	for (int i = arr.size() - 1; i >= 0; i--)
	//for(int i=0;i < arr.size();i++)
	{
		int preID = arr[i].second;
		// cout<<IDToPredicate[preID]<<" "<<arr[i].first<<endl;
		for (int p = 0; p < edge[preID].size(); ++ p)
		{

			int A = edge[preID][p].first, B = edge[preID][p].second;
			int parentA = Utils::getParent(A, FA), parentB = Utils::getParent(B, FA);

			if (RANK[parentA] < RANK[parentB])
				swap(parentA, parentB);
			if (parentA != parentB)
			{
				FA[parentB] = parentA;
				SONCNT[parentA] += SONCNT[parentB];
				RANK[parentA] = max(RANK[parentA], RANK[parentB] + 1);

				//cost(preID) > limit 选不了了
				if (SONCNT[parentA] > limit)
				{
					invalid[preID] = 1;
					break;
				}
			}
		}
		if (invalid[preID])
			break;
		choice[preID] = 1;
	}

	int crossEdge = 0;
	// for(int preID=1;preID<=svCnt;preID++)
	// 	if(choice[preID]==0)cout<<preID<<"	"<<IDToPredicate[preID]<<endl,crossEdge++;
	printf("crossEdge: %d\n", crossEdge);
	printf("\n");
	unionBlock(choice, part);
}

void graph::greed3()
{
	puts("\n==========================================================================================\n");
	printf("greed3\n");
	invalid = vector<bool>(svCnt + 1, 0);
	int threshold = entityCnt * 0.0001;
	vector<int> fa(entityCnt + 1);
	vector<int> FA(entityCnt + 1);
	for (int i = 1; i <= entityCnt; i++)
		fa[i] = FA[i] = i;
	vector<int> RANK(entityCnt + 1, 0);

	//SONCNT record the size of each weakly connected component in final result
	vector<int> SONCNT(entityCnt + 1, 1);

	vector<int> choice(svCnt + 1, 0);
	vector<pair<int, int>> arr;

	// reverse -> make sure multiPres can be chosen
	for (int preID = svCnt; preID >= 1; -- preID)
	{
		// if the edge cnt of preID is in smaller than threshold, choose it as internal
		if (edge_cnt[IDToPredicate[preID]] < threshold)
		{
			for (int p = 0; p < edge[preID].size(); p++)
			{
				int A = edge[preID][p].first, B = edge[preID][p].second;
				int parentA = Utils::getParent(A, FA), parentB = Utils::getParent(B, FA);

				if (RANK[parentA] < RANK[parentB])
					swap(parentA, parentB);
				if (parentA != parentB)
				{
					FA[parentB] = parentA;
					SONCNT[parentA] += SONCNT[parentB];
					RANK[parentA] = max(RANK[parentA], RANK[parentB] + 1);
				}
			}
			choice[preID] = 1;
			internalAddOrder.push_back(preID);
		}
		else
		{
			vector<int> parent(fa);
			vector<int> sonCnt = vector<int>(entityCnt + 1, 1);

			for (int p = 0; p < edge[preID].size(); p++)
			{

				int A = edge[preID][p].first, B = edge[preID][p].second;
				int parentA = Utils::getParent(A, parent), parentB = Utils::getParent(B, parent);

				if (parentA != parentB)
				{
					parent[parentB] = parentA;
					//sonCnt record the size of each weakly connected component in intermediate result
					sonCnt[parentA] += sonCnt[parentB];
					if (sonCnt[parentA] > limit)
					{
						invalid[preID] = 1;
						printf("invalid: %d\n", preID);
						break;
					}
				}
			}
			if (invalid[preID])
				continue;

			//sorting properties by the numbers of weakly connected components
			int wcc_cnt = 0;
			for (int p = 1; p <= entityCnt; p++)
				if (Utils::getParent(p, parent) == p)
					wcc_cnt++;
			arr.push_back({wcc_cnt * edge_weight[IDToPredicate[preID]], preID});
		}
	}

	sort(arr.begin(), arr.end());
	// put as many properties as possbile into the internal properties
	for (int i = arr.size() - 1; i >= 0; i--)
	{
		int preID = arr[i].second;
		for (int p = 0; p < edge[preID].size(); p++)
		{

			int A = edge[preID][p].first, B = edge[preID][p].second;
			int parentA = Utils::getParent(A, FA), parentB = Utils::getParent(B, FA);

			if (RANK[parentA] < RANK[parentB])
				swap(parentA, parentB);
			if (parentA != parentB)
			{
				FA[parentB] = parentA;
				SONCNT[parentA] += SONCNT[parentB];
				RANK[parentA] = max(RANK[parentA], RANK[parentB] + 1);

				if (SONCNT[parentA] > limit)
				{
					invalid[preID] = 1;
					break;
				}
			}
		}
		if (invalid[preID])
			break;
		choice[preID] = 1;
		internalAddOrder.push_back(preID);
	}

	int crossEdge = 0;
	for (int preID = 1; preID <= svCnt; preID++)
		if (choice[preID] == 0)
			cout << preID << "	" << IDToPredicate[preID] << endl, crossEdge++;
	printf("crossEdge: %d\n", crossEdge);

	puts("internal choosing order: ");
	for (auto preid : internalAddOrder)		cout << preid << " : " << IDToPredicate[preid] << endl;
	puts("\n==========================================================================================\n");
	unionBlock(choice, part);
}

void graph::unionBlock(vector<int> &choice, int goal)
{
	cout << "unionBlock: " << endl;
	WCCParentVec = vector<int>(entityCnt + 1, 0);
	for (int p = 1; p <= entityCnt; ++ p)
		WCCParentVec[p] = p;
	WCCRankVec = vector<int>(entityCnt + 1, 0);
	WCCSizeVec = vector<int>(entityCnt + 1, 1);

	for (int preID = 1; preID <= svCnt; ++ preID)
	{
		if (choice[preID] == 1)
		{
			// cout << IDToPredicate[preID] << endl;
			for (int p = 0; p < edge[preID].size(); ++ p)
			{
				int parentA = Utils::getParent(edge[preID][p].first, WCCParentVec);
				int parentB = Utils::getParent(edge[preID][p].second, WCCParentVec);
				if (WCCRankVec[parentA] < WCCRankVec[parentB])
					swap(parentA, parentB);
				if (parentA != parentB)
				{
					WCCRankVec[parentA] = max(WCCRankVec[parentA], WCCRankVec[parentB] + 1);
					WCCParentVec[parentB] = parentA;
					WCCSizeVec[parentA] += WCCSizeVec[parentB];
					entityTriples[parentA] += entityTriples[parentB];
				}
			}
		}
	}

	// mergeWCC();

	// compare key: triples contained in entity p
	vector<pair<int, int>> block;
	int blockNum = 0;
	for (int p = 1; p <= entityCnt; ++ p)
		if (p == Utils::getParent(p, WCCParentVec))
			block.push_back({entityTriples[p], p}), ++ blockNum;
	printf("blockNum: %d\n", blockNum);

	sort(block.begin(), block.end());

	// use min heap to make sure the balance between parts
	priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> Q;

	for (int i = 1; i <= goal; ++ i)
		Q.push({0, i});

	// key: WCCRoot ID  value: partition ID
	vector<int> blockTogoal(entityCnt + 1, 0);
	for (int i = block.size() - 1; i >= 0; -- i)
	{
		// top = {triples contained in entity p, partition ID}
		pair<int, int> top = Q.top();
		Q.pop();
		top.first += block[i].first;

		// blockTogoal[WCCRoot] = partition ID
		blockTogoal[block[i].second] = top.second;
		Q.push(top);
	}

	while (!Q.empty())
	{
		// all triples count, partition ID
		printf("triple count of all entity in part %d before deduplicated: %d\n", Q.top().second - 1, Q.top().first);
		Q.pop();
	}

	vector<int> CNT(goal + 1, 0);

	// if(access(RDF.c_str(), 0)==-1)
	// 	mkdir(RDF.c_str(),0777);
	// ofstream outFile("/opt/workspace/PCP/"+RDF+"InternalPoints.txt");
	ofstream outFile(RDF + "InternalPoints.txt");
	for (int pos, p = 1; p <= entityCnt; ++ p)
	{
		string t = IDToEntity[p];
		pos = blockTogoal[Utils::getParent(p, WCCParentVec)];
		int groupID = pos - 1;
		group[t] = groupID;
		outFile << t << "	" << groupID << endl;
		CNT[pos]++;
	}
	outFile.close();
	puts("");
	for (int i = 1; i <= goal; ++ i)
		printf("entity count of part %d : %d\n", i, CNT[i]);

	// ofstream File("/opt/workspace/PCP/"+RDF+"crossingEdges.txt");
	ofstream File(RDF + "crossingEdges.txt");
	File << "Predicate"
		 << "\t"
		 << "Edge Count"
		 << "\t"
		 << "IsCrossEgde"
		 << "\t"
		 << "Weight" << endl
		 << endl;
	for (unordered_map<string, int>::iterator it = edge_cnt.begin(); it != edge_cnt.end(); ++ it)
	{
		File << it->first << "\t" << it->second << "\t";
		if (predicateToID.count(it->first))
			File << (!choice[predicateToID[it->first]]);
		else
			File << "0";
		File << '\t' << edge_weight[it->first] << endl;
		File << endl;
	}

	File << "\n==========================================================================================\n" << endl;
	for (int i = preType + 1; i <= svCnt; ++i)
	{
		File << IDToPredicate[i] << "\t" << edge[i].size() << "\t";
		File << (!choice[i]) << "\t";
		File << edge_weight[IDToPredicate[i]] << endl;
		File << endl;
	}

	File.close();
	// update();
}

void graph::partition(string txt_name, string tag, string out_file)
{
	string line;
	ifstream readGraph(txt_name);
	triples = 0;

	vector<pair<pair<int, int>, string>> tmp;
	for (int i = 1; i <= part; ++i)
		edgeGroup.push_back(tmp);

	while (getline(readGraph, line))
	{
		triples++;
		if (triples % 10000 == 0)
			cout << "grouping triples : " << triples << endl;
		// line.resize(line.length() - 2);
		vector<string> s;
		s = Utils::split(line, tag);
		int u = entityToID[s[0]], v = entityToID[s[2]], p = predicateToID[s[1]];
		// if(u == 0)	continue;
		int uID = group[s[0]], vID = group[s[2]];
		// cout << triples << ":" << u << " " << v << " " << uID << ' ' << vID << ' ' << s[0] << endl;
        int new_v;
		if (v == 0)
			new_v = entityCnt + strEntityToID[s[2]];
		else
			new_v = v;
		edgeGroup[uID].push_back({{u, new_v}, s[1]});
		if (uID != vID && v != 0)
			edgeGroup[vID].push_back({{u, new_v}, s[1]});
	}

	int partCnt = part;

	cout << "entityCnt : " << entityCnt << endl;
	cout << "strEntity : " << strEntityToID.size() << endl;
	cout << "predicate : " << predicate.size() << endl;
	for (int i = 0; i < partCnt; ++i)
	{
		string SubGraphName = out_file + to_string(i) + ".txt";
		ofstream out(SubGraphName);
		int groupSize = edgeGroup[i].size();
		cout << "triples count in part " << i << " is :" << groupSize << endl;
		for (int j = 0; j < groupSize; ++j)
		{
			string s = IDToEntity[edgeGroup[i][j].first.first], p = edgeGroup[i][j].second, o;
			if (edgeGroup[i][j].first.second > entityCnt)	o = IDToStrEntity[edgeGroup[i][j].first.second - entityCnt - 1];
			else						   					o = IDToEntity[edgeGroup[i][j].first.second];
			out << s << " " << p << " " << o << " ." << endl;
			// cout << s << " " << p << " " << o << " ." << endl;
			// cout << s << " " << p << " " << edgeGroup[i][j].first.second << " ." << endl;
		}
		out.close();
	}
	// cout << "ID : " << entityToID[""] << endl;
	// cout << strEntityToID["\"A\"@en"] << endl;
	// cout << entityToID["<http://dbpedia.org/resource/Category:1809_births>"] << endl;
	// cout << entityToID["<http://dbpedia.org/resource/Category:1865_deaths>"] << endl;
}

void graph::getFileList(string template_path, string result_path)
{
	if (template_path.find_last_of("/") != template_path.size() - 1)
		template_path += "/";
	if (result_path.find_last_of("/") != result_path.size() - 1)
		result_path += "/";

	DIR *dir_1, *dir_2;
	struct dirent *ptr;
	int i = 0, cnt = 1;
	dir_1 = opendir(template_path.data());
	while ((ptr = readdir(dir_1)) != NULL)
	{
		if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0)
			continue;
		string filename = ptr->d_name;
		query_template.push_back(filename);
		queryID[filename] = cnt++;
	}
	closedir(dir_1);
	i = 0;
	dir_2 = opendir(result_path.data());
	while ((ptr = readdir(dir_2)) != NULL)
	{
		if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0)
			continue;
		string result = ptr->d_name;
		query_result.push_back(result);
	}
	closedir(dir_2);
	// cout << queryID["F5.txt"] << endl;
	queryCnt = --cnt;
	for (auto it : query_template)
		cout << it << endl;
	cout << endl;
	sort(query_result.begin(), query_result.end());
	for (auto it : query_result)
		cout << it << endl;
}

void graph::readQueryResult(string template_path, string result_path, string tag)
{
	vector<vector<string>> patternQueryNode;
	vector<string> s, s0, s1;

	// query result -> "\t"

	// initialize patternQueryNode
	vector<string> tmp;
	for (int i = 0; i < queryCnt + 1; ++i)
		patternQueryNode.push_back(tmp);
	validResultCnt = 0;
	// mkdir("./tmp/", 0775);

	string str, t;
	size_t pos = 0;
	int currPatternID;
	int id;
	//	puts("============================");
	for (auto &filename : query_result)
	{
		string multiPreName = "multiPre_" + filename;
		predicateToID[multiPreName] = IDToPredicate.size();
		currPatternID = IDToPredicate.size() - preType;

		id = queryID[filename];

		ifstream in_result((result_path + "/" + filename));

		getline(in_result, str);
		if (str == "")
			continue;
		++ validResultCnt;
		stringstream ss(str);
		while (ss >> t)
			patternQueryNode[id].push_back(t);

		ofstream out(("./tmp/" + filename + "_tmp.txt"));
		//先读结果

		vector<pair<int, int>> resultTriples;

		while (getline(in_result, str))
		{
			if (str == "")	break;
			int lineLen = str.size();

			s = Utils::split(str, "\t");

			//匹配到查询pattern

			ifstream in_template((template_path + "/" + filename));
			getline(in_template, str);
			//逐行替换
			while (getline(in_template, str))
			{
				if (str == "}")
					break;

				s0 = Utils::split(str, "\t");

				int vCnt = patternQueryNode[id].size();

				for (int i = 0; i < vCnt; ++i)
				{
					string v = patternQueryNode[id][i];
					pos = str.find(v);
					if (pos != string::npos)
					{
						str = str.replace(pos, v.size(), s[i]);
					}
				}

				str.resize(str.size() - 3);
				out << str << endl;
				s1 = Utils::split(str, "\t");
				int a = entityToID[s1[0]], b = entityToID[s1[2]];
				out << a << "<==================>" << b << endl;
				// cout << IDToEntity[a] << ' ' << IDToEntity[b] << endl;
				if (a && b)
				{
					// if (filename == "p3.txt")	cout << IDToEntity[a] << ' ' << IDToEntity[b] << endl;
					resultTriples.push_back({a, b});
					// containPattern[a][currPatternID] = 1;
					// containPattern[b][currPatternID] = 1;
				}
			}
			in_template.close();
		}
		in_result.close();
		out.close();
		edge.push_back(resultTriples);
		IDToPredicate.push_back("multiPre_" + filename);
		cout << multiPreName << " : " << currPatternID << ' ' << resultTriples.size() << endl;
	}
}
