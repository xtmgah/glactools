/*
 * GlacCAT
 * Date: Jul-30-2017 
 * Author : Gabriel Renaud gabriel.reno [at sign here ] gmail.com
 *
 */

#include "GlacCAT.h"

GlacCAT::GlacCAT(){

}

GlacCAT::~GlacCAT(){

}


string GlacCAT::usage() const{

    
    return string(string("glactools") +" cat [options] <glf file1> <glf file2> .. "+"\n"+
                  "\nThis program concatenates ACF/GLF files given that they were from the same genome assembly\n"+    
		  "uses the first file as header and prints to STDOUT\n"
		  );
}



int GlacCAT::run(int argc, char *argv[]){


    AlleleRecords * arr;
    //AlleleRecords * arw;
    GlacWriter * gw=NULL;
    string sqLines="";
    string defline="";
    uint16_t chriRead=0;
    for(int i=1;i<(argc);i++){ 
	string filename = string(argv[i]);
	GlacParser gp (filename);

	if(i==1){//first file
	    gw = new GlacWriter(gp.getSizePops(),
				false,
				2,
				1,//compression threads
				uncompressed);
	    string newheader=gp.getHeader();
	    sqLines = gp.getHeaderSQ();
	    defline = gp.getDefline();

	    if(!gw->writeHeader(newheader)){
		cerr<<"GlacCat: error writing header "<<endl;
		return 1;
	    }
	    
	    //first record
	    if(gp.hasData()){
		arr = gp.getData();
		chriRead = arr->chri;
		if(!gw->writeAlleleRecord(arr)){
		    cerr<<"GlacCAT: error writing record "<<arr<<endl;
		    exit(1);
		}
	    }

	    while(gp.hasData()){
		arr = gp.getData();

		if(!gw->writeAlleleRecord(arr)){
		    cerr<<"GlacCAT: error writing record "<<arr<<endl;
		    exit(1);
		}
	    }

	}else{
	    if(sqLines != gp.getHeaderSQ()){
		cerr<<"GlacCat: error the SQ lines do not match in headers in file: "<<string(argv[i])<<" does not match the ones in "<<string(argv[1])<<endl;
		return 1;
	    }
	    if(defline != gp.getDefline()){
		cerr<<"GlacCat: error the defline do not match in headers in file: "<<string(argv[i])<<" does not match the one in "<<string(argv[1])<<endl;
		return 1;
	    }

	    //first record
	    if(gp.hasData()){
		arr = gp.getData();
		if(chriRead > arr->chri){
		    cerr<<"GlacCAT: WARNING chromosomal coordinate in  "<<arr<<" is greater than in the previous file, indexing will not work"<<endl;
		    
		}
		chriRead = arr->chri;
		if(!gw->writeAlleleRecord(arr)){
		    cerr<<"GlacCAT: error writing record "<<arr<<endl;
		    exit(1);
		}
	    }

	    while(gp.hasData()){
		arr = gp.getData();
		if(!gw->writeAlleleRecord(arr)){
		    cerr<<"GlacCAT: error writing record "<<arr<<endl;
		    exit(1);
		}
	    }

	}
    }

    delete(gw);

    return 0;
}

