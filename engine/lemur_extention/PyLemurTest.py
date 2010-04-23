#!/usr/local/bin/python
"""some help output"""
import sys
#sys.path.append('../../web')
import gi
from pprint import *
import string
from optparse import OptionParser


def init():
	global cmdOptions, cmdArgs, flatfileParameterFile
	parser = OptionParser()
	parser.add_option("-d", "--doc_to_keywords", action="store_true", dest="doc_to_keywords", default=False, help="Test doc_to_keywords")
	parser.add_option("-k", "--keyword_to_docs", action="store_true", dest="keyword_to_docs", default=False, help="Test keyword_to_docs")
	parser.add_option("-i", "--indri_to_db", action="store_true", dest="indri_to_db", default=False, help="Test indri_to_db")
	parser.add_option("-c", "--show_cluster", action="store_true", dest="show_cluster", default=True, help="Test show_cluster")

	(cmdOptions, cmdArgs) = parser.parse_args()

	basePath = '/home/arcoleo/gooui/server/web/'
	extensionPath = '/home/arcoleo/gooui/server/engine/lemur_extention/'
	indexPath = basePath + 'lemurIndex/';
	clusterPath = basePath + 'cluster_index.cl'
	docno = 'TR-1';
	query = 'storm';
	buildIndexParameterFile = extensionPath + 'parameters'
	clusterParameterFile = extensionPath + 'cluster.parameters'
	flatfileParameterFile = basePath + 'flatfile.parameters'

# print '\nTesting buildIndex\n'
# pylemur.buildIndex(buildIndexParameterFile);

	# print "Using index [", indexPath, "]"

def test_indri_to_db():
	print '\nTesting indriToDbId'
	for doc in range(100, 200):
		print "\t", "TR-"+str(doc), ":",
		try:
			dbno = pylemur.indriToDbId(indexPath, 'TR-' + str(doc))
		except Error, e:
			print "Exception", e
		print dbno


def test_doc_to_keywords():
	print 'Testing docToKeywords: ', docno
	print pylemur.docToKeywords(indexPath, pylemur.indriToDbId(indexPath, docno))


def test_keyword_to_docs():
	print '\nTesting keywordToDocs(', query, ')\n'
	try:
		dcs = pylemur.keywordToDocs(indexPath, query)
		print dcs, '\n'
	except Error, e:
		print (Error, e)
	#print map(string.replace, dcs, '', 'TR-')
	for article in dcs:
		dcs[dcs.index(article)] = article[3:]
	print dcs


def test_clusters(cluster_type='flat_clusters'):
	global cluster
	cluster = {}
	print 'Testing clusters'
	try:
		cluster = gi.clusterType(cluster_type)
		#clstr = pylemur.showClusters('cluster.parameters')
		#print clstr[1][1:], '\n'
	
		# transpose and convert [(1,'a'),(2,'b),...] -> [[1,2,...],[a,b,...]]
		#taglist = map(list, zip(*clstr[1][1:]))
		#print taglist[1], '\n'
		#for tag in taglist[1]:
		#	print tag, ': ', pylemur.docToKeywords(indexPath, pylemur.indriToDbId(indexPath, tag)), '\n'
	
	except Exception, ex:
		print "Error in cluster: ", ex


def run_tests():
	if (cmdOptions.indri_to_db):
		test_indri_to_db()
	if (cmdOptions.doc_to_keywords):
		test_doc_to_keywords()
	if (cmdOptions.keyword_to_docs):
		test_keyword_to_docs()
	if (cmdOptions.show_cluster):
		test_clusters('flat_clusters')

init()
run_tests()
