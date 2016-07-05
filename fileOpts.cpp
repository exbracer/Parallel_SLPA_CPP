//============================================================================
// Name        : fileOpts.cpp
// Author      : Jierui Xie (xiej2@rpi.edu)
// Date        : Oct. 2011
// Version     : v1.0
// Copyright   : All rights reserved.
// Description : SLPA algorithm for community detection.
// Web Site    : https://sites.google.com/site/communitydetectionslpa/
// Publication:
//             J. Xie, B. K. Szymanski and X. Liu, "SLPA: Uncovering Overlapping Communities in Social Networks via A Speaker-listener Interaction Dynamic Process", IEEE ICDM workshop on DMCCI 2011, Vancouver, CA.
//============================================================================

#include "fileOpts.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <string>
#include <stdlib.h>
using namespace std;

bool isFileExist(const char* fileName){
	fstream fp;
	fp.open(fileName,fstream::in);//|fstream::out|fstream::app

	if(fp.is_open()){
		fp.close();
		return true;
	}else
		return false;
}

void writeToTxt(string fileName, bool isappend, vector<string>& data){
	//it is good way to do and similar to the c style.

	fstream fp;

	if(isappend)
		fp.open(fileName.c_str(),fstream::app);//|fstream::out|fstream::app
	else
		fp.open(fileName.c_str(),fstream::out);//|fstream::out|fstream::app

	if(fp.is_open()){//if it does not exist, then failed

		for(int i=0;i<data.size();i++){
			fp<<data[i]<<endl;
		}
		fp.close();

		//cout<<"write to "<<fileName<<endl;
	}
	else{
		cout<<"open failed"<<endl;
	}
}

void extractFileName_FullPath (const string& str,string& shortfile, string& file, string& path)
{
	size_t found;
	found=str.find_last_of("/\\"); //any character

	//get the path and full filename
	if(found==string::npos){
		path="";
		file=str;
	}
	else{
		path=str.substr(0,found);
		file=str.substr(found+1);
	}

	//get the short file name
	found=file.find_last_of("."); //any character
	if(found==string::npos){
		shortfile=file;
	}
	else{
		shortfile=file.substr(0,found);
	}

	//cout<<"p="<<path<<" f="<<file<<" sf="<<shortfile<<endl;
}




