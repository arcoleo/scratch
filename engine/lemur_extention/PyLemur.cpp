#ifdef MACOSX
// #include "/Library/Frameworks/Python.framework/Versions/2.5/include/python2.5/Python.h"
#include "/Developer/SDKs/MacOSX10.5.sdk/System/Library/Frameworks/Python.framework/Headers/Python.h"

#else
#include "/usr/include/python2.5/Python.h"
#endif
#include <iostream>
#include <fstream>
#include <assert.h>

// docToKeywords
#include <Index.hpp>
#include <IndexManager.hpp>

// docToIndriKeywords
#include <indri/QueryEnvironment.hpp>

// keywordsToDocs
#include <ScoreAccumulator.hpp>
#include <RetrievalMethod.hpp>
#include <IndexedReal.hpp>
#include <StringQuery.hpp>
#include <TFIDFRetMethod.hpp>
#include <DocInfoList.hpp>
#include <indri/CompressedCollection.hpp>

// buildIndex
#include <TextHandlerManager.hpp>
#include <IndriTextHandler.hpp>
#include <KeyfileTextHandler.hpp>
#include <KeyfileIncIndex.hpp>
#include <Param.hpp>
#include <indri/Path.hpp>

// Cluster
#include <FlatFileClusterDB.hpp>
#include <KeyfileClusterDB.hpp>
#include <ClusterParam.hpp>
#include <Cluster.hpp>
#include <ClusterDB.hpp>


#define DEBUG
//#define DEBUGLEVEL 101


using namespace std;

// docToKeywords
using namespace lemur::api;

// keywordsToDocs
using namespace lemur::api;
using namespace lemur::retrieval;
using namespace lemur::parse;

// buildIndex

// cluster
using namespace lemur::api;
using namespace lemur::cluster;

// Local parameters used by the indexer 
namespace LocalParameter {
	int memory;

	// index variables
	
	// name (minus extension) of the database
	string index;
	// type of index to build
	string indexType;
	// name of file containing stopwords
	string stopwords;
	// name of file containing acronyms
	string acronyms;
	// format of documents (trec or web)
	string docFormat;
	// whether or not to stem
	string stemmer;
	// file with source files
	string dataFiles;

	bool countStopWords;


	// cluster variables
	string clusterDBType;
	string databaseIndex;
	string clusterIndex;
	string threshold;
	string simType;
	string clusterType;
	string docMode;

	// index parameter reader
	void get() {
		index = ParamGetString("index");
		indexType = ParamGetString("indexType");
		memory = ParamGetInt("memory", 128000000);
		stopwords = ParamGetString("stopwords");
		acronyms = ParamGetString("acronyms");
		docFormat = ParamGetString("docFormat");
		dataFiles = ParamGetString("dataFiles");
		stemmer = ParamGetString("stemmer");
		countStopWords = (ParamGetString("countStopWords", "false") == "true");

#if DEBUGLEVEL > 100
		cerr << "index:[" << index << "]" << endl << 
			"indexType:[" << indexType << "]" << endl << 
			"memory:[" << memory << "]" << endl << 
			"stopwords:[" << stopwords << "]" << endl << 
			"acronyms:[" << acronyms << "]" << endl << 
			"docFormat:[" << docFormat << "]" << endl << 
			"dataFiles:[" << dataFiles << "]" << endl << 
			"stemmer:[" << stemmer << "]" << endl;
#endif
	}
};




static char pylemur_doc[] = 
"This module is just a simple example.  It provides one function: addtwo().";

/* -------------------------------- addto -------------------------------- */

static char pylemur_addtwo_doc[] = 
"addtwo(a, b)\n\
\n\
Return the sum of a and b.";

static PyObject*
pylemur_addtwo(PyObject *self, PyObject *args)
{
	PyObject *a, *b;
	
	if (!PyArg_UnpackTuple(args, "addtwo", 2, 2, &a, &b)) {
		return NULL;
	}

	return PyNumber_Add(a, b);
}


/* ----------------------------- indri_to_db ----------------------------- */


static char pylemur_indri_to_db_doc[] =
"indri_to_db(index_path, docno)\n\
\n\
Returns the db id of the indri docno.";

int cpp_indri_to_db(const char *indexPath, const char *docno)
{
	std:string docnoName = "docno";
	std::vector<std::string> docnoValues;

	docnoValues.push_back(docno);
	indri::api::QueryEnvironment env;
	env.addIndex(indexPath);
	std::vector<lemur::api::DOCID_T> result = env.documentIDsFromMetadata(docnoName, docnoValues);
	if (result.size() == 0)
		//LEMUR_THROW(LEMUR_IO_ERROR, "No document exists with docno: " + docno);
		return -1;

	return result[0];
}

static PyObject*
pylemur_indri_to_db(PyObject *self, PyObject *args)
{
	PyObject *a, *b;
	char tmp[1024+1];

	/* convert passed paramters to c types */
	char *indexPath;
	char *docno;

	if (!PyArg_ParseTuple(args, "ss", &indexPath, &docno)) {
		return NULL;
	}

	return Py_BuildValue("i", cpp_indri_to_db(indexPath, docno));
}


/* ---------------------------- keyword_to_docs ---------------------------- */


static char pylemur_keyword_to_docs_doc[] =
"keyword_to_docs(indexPath, query)\n\
\n\
Returns a list of doc numbers that contain the query";

static PyObject*
pylemur_keyword_to_docs(PyObject *self, PyObject *args)
{	// keywordToDocs '+indexPath+' "'+query+'"','r'

	/* convert passed paramters to c types */
	char *indexPath;
	char *query;

	std::vector< lemur::api::DOCID_T > docIDVec;
	indri::api::QueryEnvironment env;
	lemur::api::DOCID_T documentID;
	std::string docnoName = "docno";

	if (!PyArg_ParseTuple(args, "ss", &indexPath, &query)) {
		return NULL;
	}

	Index *ind = IndexManager::openIndex(indexPath);
	env.addIndex(indexPath);
	
	ArrayAccumulator accumulator(ind->docCount());
	
	RetrievalMethod *myMethod = new TFIDFRetMethod(*ind, accumulator); 
	IndexedRealVector results; 

	StringQuery *q = new StringQuery(query); // construct a TextQuery
	QueryRep * qr = myMethod->computeQueryRep(*q); // compute the query representation 

    // now score all documents    
	myMethod->scoreCollection(*qr, results);
	results.Sort(); // sorting results, assume a higher score means more relevant
	IndexedRealVector::iterator it;
	it = results.begin();

	// for converting TermInfo to str
	ostringstream buffStream;

	PyObject *pyDocList = PyList_New(0);
	
	//ofstream dbfp("pylemur-debug.txt");
	//dbfp << "Testing" << endl;

	while (it != results.end() ) {
		//documentID = atoi( (*it).ind );
		docIDVec.clear();
		docIDVec.push_back((*it).ind);
		
		std::vector< std::string > titleStrings = env.documentMetadata(docIDVec, docnoName);
		
		if (titleStrings.size() > 0) {
			buffStream << *(titleStrings.begin()) << flush;
		
			PyList_Append(pyDocList,
				Py_BuildValue("s", buffStream.str().c_str() ));
		}
			
		buffStream.str("");
		it++;
    }

	delete ind;
	//dbfp.close();

	return pyDocList;
}


/* ---------------------------- doc_to_keywords ---------------------------- */


static char pylemur_doc_to_keywords_doc[] =
"doc_to_keywords(indexPath, docno)\n\
\n\
Returns an ordered list of {occurance, word} tuples from the specified document\
 number.";

static PyObject*
pylemur_doc_to_keywords(PyObject *self, PyObject *args)
{	// ROOTPATH+'docToKeywords '+indexPath+' '+str(docno)

	/* convert passed paramters to c types */
	char *indexPath;
	int docno;
	
	if (!PyArg_ParseTuple(args, "si", &indexPath, &docno)) {
		return NULL;
	}
		
	lemur::api::Index *ind;
	
	ind = IndexManager::openIndex(indexPath);
	
	// this gets the nth document, not the document w/ id=n -> FIXME
	TermInfoList *termList = ind->termInfoList(docno);
	// TermInfoList *termList = ind->docID(docno);
	
	// iterate over entries in termList, i.e., all words in the document
	termList->startIteration();
	
	TermInfo *tEntry;
	PyObject *pyTermList = PyList_New(0);
	
	// for converting TermInfo to str
	ostringstream buffStream;
	
	while (termList->hasMore()) {
		tEntry = termList->nextEntry();
 		//cout << tEntry->count() << " " << (ind->term(tEntry->termID())) << endl;
		
		// convert termID to c++ string
		buffStream << (ind->term(tEntry->termID())) << flush;
		
		PyList_Append(pyTermList,
			Py_BuildValue("(is)", tEntry->count(), buffStream.str().c_str()));
		
		// clear buffer.  Not the best way, but good enough for now.
		buffStream.str("");
	}
	
	delete ind;
	
	return pyTermList;
	
}


/* ----------------------------- build_index ----------------------------- */


static char pylemur_build_index_doc[] =
"build_index(param_file)\n\
\n\
build an index.";

// get application parameters
void GetAppParam() {
  LocalParameter::get();
}


static PyObject*
pylemur_build_index(PyObject *self, PyObject *args)
{	// buildIndex paramfile datafile1 datafile2 ...
	int argc = 2;
	char *paramfile;
	
	if (!PyArg_ParseTuple(args, "s", &paramfile)) {
		return NULL;
	}
	
#if DEBUGLEVEL > 100
	cerr << "paramfile:[" << paramfile << "]" << endl;
#endif
	
	ParamPushFile(paramfile);
	
	LocalParameter::get();
	
	if ((argc < 3) && LocalParameter::dataFiles.empty()) {
		//usage(argc, argv);
		cout << "See BuildIndex usage" << endl;
		return Py_BuildValue("i", -1);
	}

	// Cannot create anything without Index name
	if (LocalParameter::index.empty()) {
		LEMUR_THROW(LEMUR_MISSING_PARAMETER_ERROR, "Please provide a name for the index you want to build. \nCheck the \"index\" parameter.");
	}

	if (LocalParameter::indexType.empty()) {
		LEMUR_THROW(LEMUR_MISSING_PARAMETER_ERROR, "Please provide a type for the index you want to build. \nCheck the \"indexType\" parameter. \nValid values are \"key\", or \"indri\" ");
	}

	// Create the appropriate parser and acronyms list if needed
	Parser * parser = NULL;
	parser = TextHandlerManager::createParser(LocalParameter::docFormat, LocalParameter::acronyms);
	
	// if failed to create parser, abort
	if (!parser) {
		LEMUR_THROW(LEMUR_MISSING_PARAMETER_ERROR, "Please use a valid value for the required parameter \"docFormat\". Valid values are \"trec\", \"web\", \"reuters\",\"chinese\", \"chinesechar\", and \"arabic\". See program usage or Lemur documentation for more information.");
	}

	// Create the stopper if needed.
	Stopper * stopper = NULL;
	try {
		stopper = TextHandlerManager::createStopper(LocalParameter::stopwords);
	} catch (Exception &ex) {
		ex.writeMessage();
		cerr << "WARNING: BuildIndex continuing without stop words file loaded." << endl << "To omit stop words, check the \"stopwords\" parameter." << endl;
	}

	// Create the stemmer if needed.
	Stemmer * stemmer = NULL;
	try {
		stemmer = TextHandlerManager::createStemmer(LocalParameter::stemmer);
	} catch (Exception &ex) {
		ex.writeMessage();
		cerr << "WARNING: BuildIndex continuing without stemmer." << endl << "To use a stemmer, check the \"stemmer\" and other supporting parameters." << endl << "See program usage or Lemur documentation for more information.";
	}

	TextHandler* indexer;
	lemur::index::KeyfileIncIndex* index = NULL;

	if (LocalParameter::indexType == "indri") {
		indexer = new lemur::parse::IndriTextHandler(LocalParameter::index,
			LocalParameter::memory, parser);
	} else if (LocalParameter::indexType == "key") {
		index = new lemur::index::KeyfileIncIndex(LocalParameter::index,
			LocalParameter::memory);
		indexer = new lemur::parse::KeyfileTextHandler(index,
			LocalParameter::countStopWords);
	} else {
		LEMUR_THROW(LEMUR_BAD_PARAMETER_ERROR,"Please use a valid value for the required parameter \"IndexType\". \nValid values are \"key\" or \"indri\"See program usage or Lemur documentation for more information.");
	}

	// chain the parser/stopper/stemmer/indexer
	TextHandler * th = parser;

	if (stopper != NULL) {
		th->setTextHandler(stopper);
		th = stopper;
	}

	if (stemmer != NULL) {
		th->setTextHandler(stemmer);
		th = stemmer;
	}

	th->setTextHandler(indexer);

	// parse the data files
	if (!LocalParameter::dataFiles.empty()) {
		if (!indri::file::Path::exists(LocalParameter::dataFiles)) {
			LEMUR_THROW(LEMUR_IO_ERROR, "\"dataFiles\" specified does not exist");
		}
		ifstream source(LocalParameter::dataFiles.c_str());
		if (!source.is_open()) {
			LEMUR_THROW(LEMUR_IO_ERROR,"could not open \"dataFiles\" specified");
		} else {
			string filename;
			while (getline(source, filename)) {
				cerr << "Parsing file: " << filename <<endl;
				try {
					parser->parse(filename);
				} catch (Exception &ex) {
					LEMUR_RETHROW(ex,"Could not parse file");
				}
			}
		}
	} else {
			cout << "no argv passing" << endl;
	}
	
	// free memory
	delete(indexer);
	delete(stemmer);
	delete(stopper);
	delete(parser);
	if (index)
		delete(index);
		
	return Py_BuildValue("i", 0);
}


/* ----------------------------- show_clusters ----------------------------- */


static char pylemur_show_clusters_doc[] =
"show_clusters(param_file)\n\
\n\
Returns a list of clusters.";

static PyObject*
pylemur_show_clusters(PyObject *self, PyObject *args)
{
	char *paramFile;

	if (!PyArg_ParseTuple(args, "s", &paramFile)) {
		return NULL;
	}

#if DEBUGLEVEL > 100
	cerr << endl << "paramFile:[" << paramFile << "]" << endl;
#endif

	ParamPushFile(paramFile);
	ClusterParam::get();

	Index *myIndex;
	PyObject *pyMasterDocList = PyList_New(0);
//	cerr << "Opening index 1:[" << ClusterParam::databaseIndex << ", "
//		<< ClusterParam::clusterDBType << "]" << endl;
	try {
		myIndex = IndexManager::openIndex(ClusterParam::databaseIndex);
	} catch (Exception &ex) {
		ex.writeMessage();
		throw Exception("Cluster", "Can't open index, check parameter index");
	}
//	cerr << "Opening index 2:[" << ClusterParam::databaseIndex << "]" << endl;
	try {
		lemur::api::ClusterDB *clusterDB;

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
			// convert to exception
			cout << "Cluster database type '" << ClusterParam::clusterDBType
				<< "' is not supported.\n";
			exit(1);
		}

		// parse collection
		double score;
		lemur::cluster::Cluster *currCluster;
		vector <lemur::api::DOCID_T> docsInCluster;
		lemur::api::Index *ind;
		ind = IndexManager::openIndex(ClusterParam::databaseIndex);
		ostringstream buffStream;

	//	EXDOCID_T 

		cerr << "Number Clusters: " << clusterDB->countClusters() << endl;
		for (DOCID_T clusterLoop = 1; clusterLoop <= clusterDB->countClusters(); clusterLoop++) {
			PyObject *pySubDocList = PyList_New(0);
			// convert all this to returned data, no printing here
			// cout << endl << "Cluster # " << clusterLoop << ": ";
			PyList_Append(pySubDocList, Py_BuildValue("i", clusterLoop));

			currCluster = clusterDB->getCluster(clusterLoop);
			docsInCluster = currCluster->getDocIds();
			for (int docLoop = 0; docLoop < docsInCluster.size(); docLoop++) {
				//cout << docsInCluster[docLoop];
				buffStream << (ind->document(docsInCluster[docLoop])) << flush;
				PyList_Append(pySubDocList, 
					Py_BuildValue("(is)", docsInCluster[docLoop], 
					buffStream.str().c_str()));
				buffStream.str("");
			}
			//cerr << endl << "Appending: " << pySubDocList << endl;
			PyList_Append(pyMasterDocList, pySubDocList);
		}
		//cout << "Number of clusters: " << clusterDB->countClusters() << endl;
		delete clusterDB;
	} catch (lemur::api::ClusterDBError x) {
		cerr << "Problem with clustering database: " << x.what() << endl;
	}
	delete myIndex;
	return pyMasterDocList;


}


/* ---------------------------- flat_clusters ---------------------------- */


static char pylemur_flat_clusters_doc[] =
"flat_clusters(param_file)\n\
\n\
build an index.";


namespace LocParam {
	static lemur::utility::String index;
	double lambda_b;
	int cNum; 
	
	int trial,it;
	void get() {
		index = ParamGetString("index");
		lambda_b = ParamGetDouble("lambdaB",0.4);
		cNum = ParamGetInt("cluster",5);
		trial = ParamGetInt("trial",20);
		it = ParamGetInt("it",50);
	}
};


inline double cpp_colPr(Index &ind, int t)
{
	return (1.0 + ind.termCount(t)) / (double)(ind.termCountUnique() + ind.termCount());
}


static PyObject*
pylemur_flat_clusters(PyObject *self, PyObject *args)
{
	int argc = 2;
	char *paramfile;
	
	if (!PyArg_ParseTuple(args, "s", &paramfile)) {
		return NULL;
	}
	
	ParamPushFile(paramfile);
	
	LocParam::get();
	
	// cout << endl << "Running pylemur_flat_clusters(" << paramfile << ")" << endl;
	
	PyObject *pyWordMasterDocList = PyList_New(0);
	PyObject *pyArticleMasterDocList = PyList_New(0);
	PyObject *pyMasterDocList = PyList_New(0);
	
	Index * ind;
	try {
		ind = IndexManager::openIndex(LocParam::index);
	} catch (Exception &ex) {
		ex.writeMessage();
		cerr << "Cannot open index" << endl;
		return Py_BuildValue("(i)", -1);
	}
	
	// cout << "Opened index" << endl << flush;

	int wN;
	int cN;
	int dN;
	// cout << "DSA: 1" << endl << flush;
	try {
		wN = ind->termCountUnique();
	} catch (Exception &ex) {
		ex.writeMessage();
		cout << "termCountUnique failed." << endl << flush;
		return Py_BuildValue("(i)", -1);
	}
	// cout << "DSA: 2" << endl << flush;
	try {
		cN = LocParam::cNum;
	} catch (Exception &ex) {
		ex.writeMessage();
		cout << "cNum failed." << endl << flush;
		return Py_BuildValue("(i)", -1);
	}
	// cout << "DSA: 3" << endl << flush;
	try {
		dN = ind->docCount();
	} catch (Exception &ex) {
		ex.writeMessage();
		cout << "docCount failed." << endl << flush;
		return Py_BuildValue("(i)", -1);
	}

	
	// cout << "Counted." << endl << flush;
	
	int k, i;
	
	double lambdaB = LocParam::lambda_b; // background
	
	int d;
	
	// double **pi = new (double *)[dN+1]; // class probability
	double **pi;
	pi = new double*[dN + 1];
	
	// double **c= new (double *)[cN]; // component models
	double **c;
	c = new double*[cN];
	
	// double **piEst = new (double *)[dN+1]; // class probability
	double **piEst;
	piEst = new double*[dN + 1];
	
	// double **cEst= new (double *)[cN]; // component models
	double **cEst;
	cEst = new double*[cN];
	
	for (k = 0; k < cN; k++) {
		c[k] = new double[wN + 1]; // reserve 0-index as normalizer
		cEst[k] = new double[wN + 1]; // reserve 0-index as normalizer
	}
	
	for (d = 1; d <= dN; d++) {
		pi[d] = new double[cN + 1];
		piEst[d] = new double[cN + 1];
	}
	
	int trial=0;
	
	double curFit, meanFit, bestFit;
	meanFit = 1e-40;
	
	double *z = new double[cN + 1];
	double *tmpZ = new double[cN + 1];
	
	//  do {
    trial++;
    for (k = 0; k < cN; k++) {
		cEst[k][0] = 0;
		for (i = 1; i <= wN; i++) {
			cEst[k][i] = 0.1 / wN + 0.9 * drand48();
			cEst[k][0] += cEst[k][i];
		}
    }
    
    for (d = 1; d <= dN; d++) {
		piEst[d][0] = 0;
		for (k = 1; k <= cN; k++) {
			piEst[d][k] = 0.9 / cN + 0.1 * drand48();
			piEst[d][0] += piEst[d][k];
		}
    }
	
    double sc = 0.0000001;
	
    int it = 0;
    meanFit = 1e-40;
    do { // EM
		// cout << "meanfit, " << it << endl << flush;
		it++;
		curFit = 0;
		//      cerr<< "---\n";
		// parameter estimation
		
		// class prob
		for (d = 1; d <= dN; d++) {
			for (k = 1; k <= cN; k++) {
				pi[d][k] = (sc + piEst[d][k]) / (sc * cN + piEst[d][0]);
				//  cout << "pi="<< pi[d][k] << " e:"<< piEst[d][k] << " sum:"<< piEst[d][0]<<endl;
				piEst[d][k] = 0;
			} 
			piEst[d][0] = 0;
		}
		
		// component model
		for (k = 0; k < cN; k++) {
			for (i = 1; i <= wN; i++) {
				c[k][i] = (cEst[k][i] + sc) / (sc * wN + cEst[k][0]);
				cEst[k][i] = 0;
			}
			cEst[k][0]=0;
		}
		
		TermInfoList *tList;
		
		// compute z
		double pw, zc, zs;
		TermInfo *info;
		double fq;
		int id;
		for (d = 1; d <= dN; d++) {
			tList = ind->termInfoList(d);
			tList->startIteration();
			
			double llnorm=0;
			while (tList->hasMore()) {
				info = tList->nextEntry();
				fq = info->count();
				//id = info->id();
				id = info->termID();
				double zsum=0;
				for (k = 0; k < cN; k++) {
					z[k] = (1 - lambdaB) * pi[d][k+1] * c[k][id];
					//   cout << "l="<<lambdaB << " pi="<< pi[d][k+1] <<" c="<< c[k][id]<<endl;
					zsum += z[k];
				}
				zsum += lambdaB * cpp_colPr(*ind, id);
				//  cout << zsum << endl; 
				curFit += fq * log(zsum);
				zsum = 1.0 / zsum;
				for (k = 0; k < cN; k++) {
					z[k] = z[k] * zsum;
				}
				
				for (k = 0;k < cN; k++) {
					piEst[d][k+1] += z[k];
					piEst[d][0] += z[k];
					cEst[k][id] += z[k] * fq;
					cEst[k][0] += z[k] * fq;
				}
			}
			delete tList;
		}
		meanFit = 0.1 * meanFit + 0.9 * curFit;
		// cout << "*** current fit:" << curFit <<endl;
    } while (fabs((meanFit - curFit) / meanFit) > 0.0000001 && it < LocParam::it);
    if (trial == 1 || curFit > bestFit) bestFit = curFit;
    // cout << "######### current: " <<curFit << "  best: "<< bestFit << endl;
    //  } while ( trial <LocParam::trial || curFit < bestFit );
	
	int n;
	IndexedRealVector temp;
	IndexedRealVector::iterator itr;
	ostringstream buffStream;
	
	for (k = 0; k < cN; k++) {
		double sum = 0;
		PyObject *pySubDocList = PyList_New(0);
		PyObject *pySubSubDocList = PyList_New(0);
		
		for (d = 1; d <= dN; d++) {
			sum += pi[d][k+1];
		}
		// cout << "****** cluster " << k <<  " docprob: " << sum / dN << " *********\n";
		PyList_Append(pySubDocList, Py_BuildValue("(id)", k, sum / dN));
		temp.clear();
		for (i = 1; i <= wN; i++) {
			temp.PushValue(i, c[k][i]);
		}
		temp.Sort();
		itr = temp.begin();
		for (n = 1; n <= 50; n++) {
			if (itr == temp.end())
				break;
			buffStream << ind->term((*itr).ind) << flush;
			// PyList_Append(pySubDocList, Py_BuildValue("(is)", docsInCluster[docLoop], buffStream.str().c_str()));
			
			//cout <<  ind->term((*itr).ind) << " " << (*itr).val << endl;
			PyList_Append(pySubSubDocList, Py_BuildValue("(sd)", buffStream.str().c_str(), (*itr).val));
			buffStream.str("");
			//      c[k][(*itr).ind]=-1;
			itr++;
		}
		PyList_Append(pySubDocList, pySubSubDocList);
		PyList_Append(pyWordMasterDocList, pySubDocList);
	}
	// cout << "#### FINAL FIT: " << curFit << endl;
	
	for (d = 1; d <= dN; d++) {
		//PyObject *pySubDocList = PyList_New(0);
		int maxC = 0;
		double maxP;
		
		for (i = 1; i <= cN; i++) {
			if (i == 1) {
				maxC = i;
				maxP = pi[d][i];
			} else if (pi[d][i] > maxP) {
				maxC = i;
				maxP = pi[d][i];
			}
		}
		// cout << ind->document(d) << " cluster " << maxC << " " << maxP << endl;
		buffStream << ind->document(d) << flush;
		PyList_Append(pyArticleMasterDocList, Py_BuildValue("sid", buffStream.str().c_str(), maxC, maxP));
		buffStream.str("");
		
	}
	//PyList_Append(pyMasterDocList, pyWordMasterDocList);
	//PyList_Append(pyMasterDocList, Py_BuildValue("{sO}", "cluster_words", pyWordMasterDocList));
	//PyList_Append(pyMasterDocList, pyArticleMasterDocList);
	//PyList_Append(pyMasterDocList, Py_BuildValue("{sO}", "cluster_articles", pyArticleMasterDocList));
	
	//cerr << "#### FINAL FIT: " << curFit << endl;
	delete ind;
	return Py_BuildValue("{sOsO}", "cluster_words", pyWordMasterDocList, 
		"cluster_articles", pyArticleMasterDocList);
	
	//return pyMasterDocList;
	

}
/* ------------------------------- pylemur ------------------------------- */

static PyMethodDef pylemur_methods[] = {
	{"addtwo", pylemur_addtwo, METH_VARARGS, pylemur_addtwo_doc},
	{"indri_to_db", pylemur_indri_to_db, METH_VARARGS, pylemur_indri_to_db_doc},
	{"doc_to_keywords", pylemur_doc_to_keywords, METH_VARARGS, pylemur_doc_to_keywords_doc},
	{"keyword_to_docs", pylemur_keyword_to_docs, METH_VARARGS, pylemur_keyword_to_docs_doc},
	{"build_index", pylemur_build_index, METH_VARARGS, pylemur_build_index_doc},
	{"show_clusters", pylemur_show_clusters, METH_VARARGS, pylemur_show_clusters_doc},
	{"flat_clusters", pylemur_flat_clusters, METH_VARARGS, pylemur_flat_clusters_doc},
	{NULL, NULL}
};

#ifndef PyMODINIT_FUNC
#define PyMODINIT_FUNC void
#endif

PyMODINIT_FUNC
initpylemur(void)
{
	PyObject *m;
	m = Py_InitModule3("pylemur", pylemur_methods, pylemur_doc);
	if (m == NULL)
		return;
}
