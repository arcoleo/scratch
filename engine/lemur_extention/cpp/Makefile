all: docToKeywords keywordToDocs showClusters

docToKeywords: docToKeywords.cpp
	g++ -DP_NEEDS_GNU_CXX_NAMESPACE=1 -o docToKeywords docToKeywords.cpp -lpthread -llemur -lm -lz

keywordToDocs: keywordToDocs.cpp
	g++ -DP_NEEDS_GNU_CXX_NAMESPACE=1 -o keywordToDocs keywordToDocs.cpp -lpthread -llemur -lm -lz

showClusters: showClusters.cpp
	g++ -DP_NEEDS_GNU_CXX_NAMESPACE=1 -g -o showClusters showClusters.cpp -lpthread -llemur -lm -lz

clean:
	rm -rf docToKeywords keywordToDocs showClusters