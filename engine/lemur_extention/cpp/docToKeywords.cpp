#include <Index.hpp>
#include <IndexManager.hpp>

using namespace lemur::api;

// ROOTPATH+'docToKeywords '+indexPath+' '+str(docno)
int main(int argc, const char* argv[])
{
	Index *ind;
	
	if (argc == 2) {
		if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
			cout << endl << "Usage:" << endl << endl <<
				"docToKeywords indexPath docno" << endl << endl << 
				"Returns an ordered list of {occurance, word} tuples from" <<
				"the specified document number." << endl << endl;
			return(0);
		}
	}

	ind = IndexManager::openIndex(argv[1]);  
  	TermInfoList *termList = ind->termInfoList(atoi(argv[2]));
	// iterate over entries in termList, i.e., all words in the document
	termList->startIteration();   
	TermInfo *tEntry;
	while (termList->hasMore()) {
		tEntry = termList->nextEntry();
 		cout << tEntry->count() << " " << (ind->term(tEntry->termID())) << endl;
	}
}