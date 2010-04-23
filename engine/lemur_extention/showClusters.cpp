#include <IndexManager.hpp>
#include <FlatFileClusterDB.hpp>
#include <KeyfileClusterDB.hpp>
#include <ClusterParam.hpp>
#include <Cluster.hpp>
#include <ClusterDB.hpp>

using namespace lemur::api;
using namespace lemur::cluster;

void GetAppParam() {
	ClusterParam::get();
}

int AppMain(int argc, char *argv[]) {
	Index *myIndex;

	try {
		myIndex = IndexManager::openIndex(ClusterParam::databaseIndex);
	} catch (Exception &ex) {
		ex.writeMessage();
		throw Exception("Cluster", "Can't open index, check parameter index");
	}
	try {
		lemur::api::ClusterBD* clusterDB;

		if (ClusterParam::clusterDBType == "keyfile") {
			clusterDB = new lemur::cluster::KeyfileClusterDB(myIndex,
				ClusterParam::clusterIndex, ClusterParam::threshold,
				ClusterParam::simType, ClusterParam::clusterType,
				ClusterParam::docMode);
		} else if (ClusterParam::clusterDBType == "flatfile") {
			clusterDB = new lemur::cluster::FlatFileClusterDB(myIndex,
				ClusterParam::clusterIndex, ClusterParam::threshold,
				ClusterParam::simType, ClusterParam::clusterType,
				ClusterParam::docMode);
		} else {
			cout << "Cluster database type '" << ClusterParam::clusterDBType
				<< "' is not supported.\n";
			exit(1);
		}

		// parse collection
		double score;
		for (DOCID_T i = 1; i <= myIndex->docCount(); i++) {
			if (clusterDB->getDocClusterId(i).size() == 0) {
				// hasn't been clustered yet

				// this program doesn't cluster documents, but if you wanted to,
				// undocument the following:
				// int myCluster = clusterDB->cluster(i, score);
				// cout << myIndex->document(i) << " " << myCluster << " "
				//	<< score << endl;

				cout << "DocID: " << myIndex->document(i) << " unclustered."
					<< endl;
			}
		}
		cout << "Number of clusters: " << clusterDB->countClusters() << endl;
		delete clusterDB;
	} catch(lemur::api::ClusterDBError x) {
		cout << "Problem with Clustering Database: " << x.what() << endl;
	}
	delete myIndex;
	return 0;
}
		
