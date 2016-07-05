//============================================================================
// Name        : NODE.h
// Author      : Jierui Xie (xiej2@rpi.edu)
// Date        : Oct. 2011
// Version     :
// Copyright   : All rights reserved.
// Description : SLPA algorithm for community detection.
// Web Site    : https://sites.google.com/site/communitydetectionslpa/
// Publication:
//             J. Xie, B. K. Szymanski and X. Liu, "SLPA: Uncovering Overlapping Communities in Social Networks via A Speaker-listener Interaction Dynamic Process", IEEE ICDM workshop on DMCCI 2011, Vancouver, CA.
//============================================================================
#ifndef NODE_H_
#define NODE_H_

#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <tr1/unordered_set>

using namespace std;

typedef std::tr1::unordered_set<int> UOrderedSet_INT;


class NODE {
public:
	int ID;
	int numNbs;
	//vector<int> nbList;
	vector<NODE *> nbList_P;   //the pointer version
	//set<int> nbSet;
	UOrderedSet_INT nbSet;

	//----------------
	//SLPA
	vector<int> WQueue;
	//map<int,int> WQHistgram;
	vector<pair<int,int> > WQHistMapEntryList;

	//----------------
	int status;
	int t; 		               //for asynchronous update

	//----------------
	NODE();

	NODE(int id){
		ID=id;

		status=0;
		t=0;
	};

	virtual ~NODE();
	//----------------


};

#endif /* NODE_H_ */
