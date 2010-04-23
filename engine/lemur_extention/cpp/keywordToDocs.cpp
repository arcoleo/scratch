#include <Index.hpp>
#include <IndexManager.hpp>
#include <ScoreAccumulator.hpp>
#include <RetrievalMethod.hpp>
#include <IndexedReal.hpp>
#include <StringQuery.hpp>
#include <TFIDFRetMethod.hpp>

using namespace lemur::api;
using namespace lemur::retrieval;
using namespace lemur::parse;

// keywordToDocs '+indexPath+' "'+query+'"','r'
int main(int argc, const char* argv[]) 
{
	if (argc == 2) {
		if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
			cout << endl << "Usage:" << endl << endl <<
				"keywordToDocs indexPath \"query\"" << endl << endl <<
				"Returns a list of doc numbers that contain the query" <<
				endl << endl;
			return(0);
		}
	}
	Index *ind = IndexManager::openIndex(argv[1]);

	ArrayAccumulator accumulator(ind->docCount());
	RetrievalMethod *myMethod = new TFIDFRetMethod(*ind, accumulator); 
	IndexedRealVector results; 
    StringQuery *q = new StringQuery(argv[2]); // construct a TextQuery
    QueryRep * qr = myMethod->computeQueryRep(*q); // compute the query representation 
    // now score all documents    
    myMethod->scoreCollection(*qr, results);
    results.Sort(); // sorting results, assume a higher score means more relevant
    IndexedRealVector::iterator it;
    it = results.begin();
    while ((it != results.end())) {
         cout << (*it).ind  // this is the document ID 
              << endl;
         it++;
    }
}