#include "graph.h"
// #include "Utils.h"
#include <ctime>
clock_t st, ed;
double endtime;

//[0]./mpc2 [1]rdf_path  [2]output_prefix [3]tag [4]k [5]pattern_template [6]pattern_result
int main(int argc, char *argv[])
{
	string rdf_name = argv[1];
	string name = argv[2];
	string sign = (string(argv[3]) == "1") ? " " : "\t";
	int part = atoi(argv[4]);
	string pattern_path = argv[5];
	string result_path = argv[6];

	st = clock();
		graph *test = new graph();
		test->init();
		test->RDF = name;
		test->part = part;

		test->loadGraph(rdf_name, sign);
		test->getFileList(pattern_path, result_path);
		test->readQueryResult(pattern_path, result_path, sign);

		int svCnt = test->getSvCnt();
		if (svCnt < 20)
			// if the number of properties is smaller than 20, we find the optimal partitioning results.
			test->unionEdgeForEnum();
		// test->greed2();
		else if (svCnt >= 20 && svCnt < 120)
			test->unionEdgeForGreed();
		else
			test->greed3();
		test -> partition(rdf_name, sign, name);

	ed = clock();
	endtime = (double)(ed - st) / CLOCKS_PER_SEC;
	cout << "partition : " << endtime << " s" << endl;

	delete test;
	return 0;
}
// 编译:g++ main.cpp -std=c++11 -o mpc2
// ./mpc2 watdiv100K.nt MPC_watdiv100K_data 2 8 ./origin_query/watdiv_query/official ./warpInput/watdiv100k_result > watdiv_100K_result.txt
// ./mpc2 watdiv100M.txt MPC_watdiv100M_data 2 8 ./origin_query/watdiv_query/official ./warpInput/watdiv100m_result

// ./mpc2 watdiv100M.txt MPC_watdiv100M_data 2 8 ./origin_query/watdiv_query/round0 ./warpInput/100M/round0_result/ > watdiv_100M_result.txt
// ./mpc2 watdiv1B.nt MPC_watdiv1B_data 2 8 ./origin_query/watdiv_query/round0 ./warpInput/round0_result > watdiv_1B_result.txt

// nohup ./mpc2 dbpedia2014.nt dbpedia2014_data 1 8 ./origin_query/dbpedia_query ./warpInput/1B/dbpedia2014_result > dbpedia2014_result.txt &
// ./mpc2 dataset/dbpedia2014_10M.nt dbpedia2014_10M_data 1 8 ./origin_query/dbpedia_query ./warpInput/1B/dbpedia2014_result
// ./mpc2 dbpedia2014_100K.nt dbpedia2014_100K_data 1 8 ./origin_query/dbpedia_query ./warpInput/1B/dbpedia2014_result > dbpedia2014_100K_result.txt