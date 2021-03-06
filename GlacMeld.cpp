/*
 * GlacMeld
 * Date: Jul-30-2017 
 * Author : Gabriel Renaud gabriel.reno [at sign here ] gmail.com
 *
 */

#include "GlacMeld.h"

GlacMeld::GlacMeld(){

}

GlacMeld::~GlacMeld(){

}

bool sortPairs(const pair<string,string> &a,const pair<string,string> &b){
    //return a.second.compare(a.second);
    return (a.second < b.second);
}

string GlacMeld::usage() const{

    
    return string("") +"glactools meld [options] <glac file> \"popToMerge1_to_1,popToMerge2_to_1,....\" \"newid1\" \"popToMerge1_to_2,popToMerge2_to_2,....\" \"newid2\"\n"+
	"This program will merge different specified populations into a single one and will print to STDOUT\n"+
	"\n"+
	"ex:  glactools glacmeld data.acf.gz \"Papuan,Austalian\" \"Oceanians\"  \"Yoruba,Mende\" \"WestAfricans\"   "+"\n"+
	"\n"+
	"Options:"+"\n"+
	"\t"+"-u" + "\t\t\t"+"Produce uncopressed output (default: "+booleanAsString(uncompressed)+")\n"+
	"\t"+"-f" + " [file]\t\t"+"Merge populations according to file in the following tab-delimited format:\n"+
	"\t"+"\t\t\t"             +"popToMerge1_to_1,popToMerge2_to_1,.... newid1\n"+
        "\t"+"\t\t\t"             +"popToMerge1_to_2,popToMerge2_to_2,.... newid2\n"+
	"\n"+
	 "\t"+ "\t\t\t"+"The file can also specify one individual/pop and the new id similar to a .ind file from EIGENSTRAAT (again tab-delimited):\n"+
	 "\t"+"\t\t\t"             +"popToMerge1 newPopid1\n"+
         "\t"+"\t\t\t"             +"popToMerge2 newPopid1\n"+
        "\n"+
	"\t-k\t\t\tKeep the original populations in the output (Default "+boolStringify(keepOrig)+" )\n";
    ;
}



int GlacMeld::run(int argc, char *argv[]){

    int lastOpt=1;
    bool specifiedFile     =false;

    string filepops;
    
    for(int i=1;i<(argc);i++){ 
        //cout<<i<<"\t"<<string(argv[i])<<endl;
        if((string(argv[i]) == "-")  ){
            lastOpt=i;
            break;          
        }
        if(string(argv[i])[0] != '-' ){
            lastOpt=i;
            break;
        }

        if(string(argv[i]) == "-f"){
            specifiedFile=true;
	    filepops     = string(argv[i+1]);
	    i++;
            continue;
        }

        if(string(argv[i]) == "-u"){
            uncompressed=true;
            continue;
        }

        if(string(argv[i]) == "-k"){
            keepOrig=true;
            continue;
        }

        cerr<<"Error unknown option "<<argv[i]<<endl;
        return 1;

    }

    // if(specifiedFile &&
    //    specifiedFile2cols){
    // 	cerr<<"Cannot specify both types of population files"<<endl;
    // 	return  1;               
    // }
    
    // if(lastOpt != (argc-3)){
    //     cerr<<"The last arguments are the  <acf file> \"popToMerge1,popToMerge2,....\" \"newid\" "<<endl;
    //     return 1;               
    // }
    // cerr<<lastOpt<<endl;
    // return 1;
    string fileglac                  = string(argv[lastOpt  ]);
    vector<string> pop2mergeString;
    vector<string> mergedpopName  ;
    unsigned int numberOfMelds=0;
    unsigned int numberOfMeldPopsOrig=0;
    if(specifiedFile){
	vector< pair<string,string> > pops;
	string line;
	igzstream myfile;
	myfile.open(filepops.c_str(), ios::in);

	if (!myfile.good()){
	    cerr << "Unable to open file "<<filepops<<endl;
	    return 1;
	}

	while ( getline (myfile,line)){   
	    vector<string> fields = allTokens( line ,'\t');
	    if(fields.size()!=2){
		cerr<<"Line "<<line<<" should have 2 TAB deliminated fields"<<endl;
		return 1;
	    }
	    pops.push_back( pair<string,string>(fields[0],fields[1]) );
	}
	
	myfile.close();
	sort(pops.begin(), pops.end(), sortPairs );
	string prepop="#";
	string preind="#";
	
	for(unsigned int i=0;i<pops.size();i++){
	    //cerr<<i<<" "<<pops[i].first<<" "<<pops[i].second<<endl;
	    if(prepop  == "#"){
		preind=pops[i].first;
		prepop=pops[i].second;
		continue;
	    }

	    if(prepop != pops[i].second){//new pop
		//cerr<<i<<" "<<preind<<" "<<prepop<<endl;
		pop2mergeString.push_back( preind  );
		mergedpopName.push_back(   prepop  );
		numberOfMelds++;       
		preind=pops[i].first;
		prepop=pops[i].second;
	    }else{//same as old pop
		preind=preind+","+pops[i].first;
		prepop=pops[i].second;
	    } 
	}
	if(prepop  != "#"){	
	    pop2mergeString.push_back( preind  );
	    mergedpopName.push_back(   prepop  );
	    numberOfMelds++;       
	}
    }else{

	
	for(int i=lastOpt;i<(argc-2);i+=2){
	    pop2mergeString.push_back( string(argv[i+1]) );
	    mergedpopName.push_back(   string(argv[i+2]) );
	    numberOfMelds++;       
	}
    }

    for(unsigned int i=0;i<pop2mergeString.size();i++){
	cerr<<"replacing: "<<pop2mergeString[i]<<" with "<<mergedpopName[i]<<endl;	
    }

    
    if(numberOfMelds<1){
        cerr<<"meld please specify a set of populations"<<endl;
        return 1;               	
    }

    for(unsigned int n1=0;n1<numberOfMelds;n1++){
	for(unsigned int n2=0;n2<numberOfMelds;n2++){
	    if(n1==n2) continue;
	    if(mergedpopName[n1] == mergedpopName[n2]){
		cerr<<"meld cannot use the same population name twice "<<mergedpopName[n1]<<endl;
		return 1;               	
	    }
	}
    }

    vector< vector<string> >    tomerge;
    vector< set<unsigned int> > indexPopTomerge;
    set<unsigned int> indexPopTomergeAll;

    for(unsigned int n=0;n<numberOfMelds;n++){
	vector<string> tomerge_ = allTokens( pop2mergeString[n],',');
	numberOfMeldPopsOrig+=tomerge_.size();
	set<unsigned int> indexPopTomerge_;
	tomerge.push_back(tomerge_);
	indexPopTomerge.push_back(indexPopTomerge_);
    }

    GlacParser gp (fileglac);
    if(	gp.isGLFormat()){
        cerr<<"meld cannot merge genotype likelihood files GLF"<<endl;
        return 1;               
    }

   
    uint32_t newsizepop= 0;
    if(keepOrig){ 
	newsizepop = gp.getSizePops()+numberOfMelds; 
    }else{ 
	newsizepop = uint32_t(int(gp.getSizePops())-int(numberOfMeldPopsOrig)+int(numberOfMelds)); 
    }

    GlacWriter * gw = new GlacWriter(newsizepop,
				     false,
				     2,
				     1,//compression threads
				     uncompressed);

    for(unsigned int n=0;n<numberOfMelds;n++){

	for(unsigned int i=0;i<gp.getPopulationsNames()->size();i++){
	    for(unsigned int j=0;j<tomerge[n].size();j++){
		if( tomerge[n][j] == gp.getPopulationsNames()->at(i) ){
		    indexPopTomerge[n].insert(i);
		    if(indexPopTomergeAll.find(i) != indexPopTomergeAll.end()){
			cerr<<"Error: A population seems to be used twice "<<gp.getPopulationsNames()->at(i)<<" "<<endl;
			return 1;
		    }

		    indexPopTomergeAll.insert(i);		    
		}
	    }
	}

	if(indexPopTomerge[n].size() != tomerge[n].size()){
	    cerr<<"Error: some of the populations to merge "<<pop2mergeString[n]<<" were not found"<<endl;
	    return 1;
	}	
    }


    string defline =    "#chr\tcoord\tREF,ALT\t";
    //for(unsigned int n=0;n<numberOfMelds;n++){
    for(unsigned int i=0;i<gp.getPopulationsNames()->size();i++){
	if( indexPopTomergeAll.find(i) ==   indexPopTomergeAll.end() ){
	    defline+=gp.getPopulationsNames()->at(i)+"\t";
	}else{
	    if(keepOrig)
		defline+=gp.getPopulationsNames()->at(i)+"\t";
	    //skip
	}
    }	

    for(unsigned int n=0;n<numberOfMelds;n++){
	defline+=vectorToString(mergedpopName,"\t")+"\n";
    }


    stringstream header;
    header<<"#ACF"<<endl;    	

    string programLine;
    for(int i=0;i<(argc);i++){ 
	programLine+=(string(argv[i])+" ");
    }
    header<<"#PG:"<<programLine<<endl;
    header<<"#GITVERSION: "<<returnGitHubVersion(argv[-1],"")<<endl;
    header<<"#DATE: "<<getDateString()<<endl;
    header<<"#GLACMELD: ";
    for(unsigned int n=0;n<numberOfMelds;n++){
	header<<pop2mergeString[n]<<" "<<mergedpopName[n]<<" ";
    }
    header<<endl;

    header<<"#MELDFILE#"<<(1)<<endl;
    header<<""<<gp.getHeaderNoSQNoDefline("#\t")<<endl;

    header<<gp.getHeaderSQ("")<<endl;
    header<<defline<<endl;

    //cout<<header.str()<<endl;
    
    if(!gw->writeHeader(header.str())){
	cerr<<"GlacMeld: error writing header "<<endl;
	exit(1);
    }

    //cerr<<"loop "<<n<<"\t"<<indexPopTomerge[n].size()<<"\t"<<tomerge[n].size()<<endl;		
    // for(unsigned int n=0;n<numberOfMelds;n++){
    // 	cerr<<"l1 "<<n<<" "<<indexPopTomerge[n].size()<<endl;
    // 	set<unsigned int>::iterator iter;
    // 	for(iter=indexPopTomerge[n].begin(); iter!=indexPopTomerge[n].end();++iter){
    // 	    cerr<<*iter<<endl;
    // 	}
    // }
    // exit(1);


    //cout<<"newsizepop "<<newsizepop<<endl;
    AlleleRecords * arr;

    while(gp.hasData()){
	//cout<<endl<<"data"<<endl;
	arr = gp.getData();
	// cout<<*arr<<endl;
	// cout<<arr->chr<<"\t"<<arr->coordinate<<"\t"<<arr->ref<<","<<arr->alt<<"\t"<<endl;

	AlleleRecords  arw;
	arw.copyCoreFields(*arr);
	arw.sizePops=newsizepop;

	vector< SingleAllele > newSingleAllele;
	for(unsigned int n=0;n<numberOfMelds;n++){
	    SingleAllele newSingleAllele_;
	    newSingleAllele.push_back(newSingleAllele_);
	}
	// newSingleAllele.refCount=0;
	// newSingleAllele.altCount=0;
	// newSingleAllele.isCpg=false;
	
	for(unsigned int i=0;i<arr->vectorAlleles->size();i++){

	    if(indexPopTomergeAll.find(i) ==   indexPopTomergeAll.end() ){//print normally if not found
		arw.vectorAlleles->push_back(arr->vectorAlleles->at(i));
	    }else{
		if(keepOrig){
		    //cout<<test->vectorAlleles->at(i) <<"\t";
		    arw.vectorAlleles->push_back(arr->vectorAlleles->at(i));
		}
		for(unsigned int n=0;n<numberOfMelds;n++){
		    if(indexPopTomerge[n].find(i) !=   indexPopTomerge[n].end() ){//add in new single allele
			newSingleAllele[n]+=arr->vectorAlleles->at(i);
		    }
		}
	    }
	}
	
	//cout<<newSingleAllele <<endl; 
	for(unsigned int n=0;n<numberOfMelds;n++){
	    arw.vectorAlleles->push_back(newSingleAllele[n]);
	}

	if(!gw->writeAlleleRecord(&arw)){
     	    cerr<<"GlacMeld: error record "<<arw<<endl;
     	    exit(1);
     	}
    }


    delete(gw);

    cerr<<"Program glactools "<<argv[0]<<"  terminated gracefully"<<endl;


    return 0;
}

