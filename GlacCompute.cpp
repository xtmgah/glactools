
#include "GlacCompute.h"


using namespace std;

typedef struct{
    char * buffer;
    unsigned int sizeRecordsRead;
} datachunk;


map<unsigned int, int>       threadID2Rank;
bool isGLF;
char sizeBytesFormat;

//queue< vector<string>  * >  * queueFilesToprocess;
//queue< vector<AlleleRecords>  * >  * queueFilesToprocess;
//queue< char                   * >  * queueFilesToprocess;
queue< datachunk  * >  * queueFilesToprocess;
//queue< char  * >  * queueFilesToprocess;

pthread_mutex_t  mutexTHREADID   = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t  mutexQueue      = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t  mutexCounter    = PTHREAD_MUTEX_INITIALIZER;
bool doneReading;


vector<string> * populationNames;
bool allowUndefined;


GlacCompute::GlacCompute(){

}

GlacCompute::~GlacCompute(){

}

template <typename STAT> //type 
void *mainComputationThread(void * argc){

    vector<STAT *> * results =  static_cast<vector<STAT *> *>( argc ) ;

    int   rc;
    // int   stackIndex;
    string freqFileNameToUse;
    int rankThread=0;
    //vector < string >  * dataToUse;
    //vector < AlleleRecords >  * dataToUse;
    datachunk   * dataToUse;

    rc = pthread_mutex_lock(&mutexTHREADID);
    checkResults("pthread_mutex_lock()\n", rc);

    threadID2Rank[*(int *)pthread_self()]  = threadID2Rank.size()+1;
    rankThread = threadID2Rank[*(int *)pthread_self()];
    
    cerr<<"Thread #"<<rankThread <<" is starting"<<endl;

    rc = pthread_mutex_unlock(&mutexTHREADID);
    checkResults("pthread_mutex_unlock()\n", rc);

 checkqueue:    
    // stackIndex=-1;
    //check stack

    // cerr<<"Thread #"<<rankThread <<" started and is requesting mutex"<<endl;

    rc = pthread_mutex_lock(&mutexQueue);
    checkResults("pthread_mutex_lock()\n", rc);


    bool foundData=false;

   
    // cerr<<"Thread #"<<rankThread <<" started and is requesting data"<<endl;
    // cerr<<"Thread #"<<(unsigned int)pthread_self() <<" started "<<endl;
    //cout<<"Thread # "<<rankThread<<"TRes "<<results<<endl;

    // cout<<"Thread "<<(unsigned int)pthread_self()<<" taking mutex queue "<<endl;
    if(!queueFilesToprocess->empty()){    
	//cout<<"Thread #"<<rankThread <<" is requesting data"<<endl;
	//cout<<"Thread "<<(unsigned int)pthread_self()<<" taking mutex queue "<<endl;
 	foundData=true;
	dataToUse = queueFilesToprocess->front();
 	queueFilesToprocess->pop();
 	//cerr<<"Thread #"<<rankThread<<" is <<endl;
    }

    //cout<<"Thread #"<<rankThread <<" is requesting data "<<foundData<<" "<<doneReading<<" "<<endl;    
  
    //if(foundData) cout<<"size data "<<dataToUse->size()<<endl;

    if(!foundData){
 	if(doneReading){

	    rc = pthread_mutex_unlock(&mutexQueue);
	    checkResults("pthread_mutex_unlock()\n", rc);

	    cerr<<"Thread #"<<rankThread<<" is done"<<endl;
	    return NULL;	
 	}else{
	    //  cout<<"Queue is empty, thread #"<<rankThread<<" will sleep for 5 seconds and wait for data"<<endl;

	    rc = pthread_mutex_unlock(&mutexQueue);
	    checkResults("pthread_mutex_unlock()\n", rc);
	    cerr<<"Queue is empty, thread #"<<rankThread<<" will sleep for 5 seconds, if this happens a lot, consider reducing the number of threads"<<endl;
 	    sleep(2);

 	    goto checkqueue;	   
 	}
    }else{
	//release stack
	rc = pthread_mutex_unlock(&mutexQueue);
	checkResults("pthread_mutex_unlock()\n", rc);
    }


    //////////////////////////////////////////////////////////////
    //                BEGIN COMPUTATION                         //
    //////////////////////////////////////////////////////////////
    //cout<<"Thread #"<<rankThread<<" is starting computations"<<endl;
    // cout<<populationNames<<" "<<vectorToString(*populationNames)<<endl;
    // cout<<"Thread #"<<rankThread<<" is starting computations2"<<endl;
    //unsigned int count=0;
    // for(unsigned int i=0;i<=dataToUse->size();i++){
    // 	//count++;
    // 	cout<<"mcp "<<i<<"\t"<<(dataToUse->at(i))<<endl;
    // }

    //cout<<populationNames<<endl;
    STAT * statComputer = new STAT(populationNames);
    // cout<<"Thread #"<<rankThread <<" addrt stat "<<statComputer<<endl;

    // for(unsigned i=0;i<dataToUse->size();i++){

    // 	//cout<<"Thread #"<<rankThread <<"  "<<i<<endl;

    // 	statComputer->computeStatSingle(&(dataToUse->at(i)),allowUndefined);

    // }
    //cout<<"Thread #"<<rankThread<<" building GP "<<dataToUse->sizeRecordsRead<<" size="<<int(sizeBytesFormat)<<" isGLF "<<isGLF<<endl;

    //cout<<"BUFFERADDR "<<(void*)dataToUse->buffer<<endl;
    GlacParser gp ( dataToUse->buffer,
		    *populationNames,
		    dataToUse->sizeRecordsRead,
		    isGLF,
		    sizeBytesFormat);

    AlleleRecords  * currentRecord;
    //cout<<"Thread #"<<rankThread<<" calling hasData() "<<endl;
    while(gp.hasData()){
	//cout<<"Thread #"<<rankThread<<" before hasData() "<<endl;
    	currentRecord = gp.getData() ;
	//cout<<"Thread #"<<rankThread<<" "<<*currentRecord<<endl;
	// cout<<"Thread #"<<rankThread<<" after hasData() "<<currentRecord->chri<<":"<<currentRecord->coordinate<<endl;
    	statComputer->computeStatSingle(currentRecord,allowUndefined);

    	//cout<<test->print()<<endl;
    }
    //cout<<"Thread #"<<rankThread<<" DONE hasData() "<<endl;    
    // //TODO set parser
    // GlacParser gp                  ("");    
    // //GlacParser gp                  (dataToUse,*populationNames);    
    // AlleleRecords  * currentRecord;

    // while(gp.hasData()){
    // 	currentRecord = gp.getData() ;

    // 	statComputer->computeStatSingle(currentRecord,allowUndefined);

    // 	//cout<<test->print()<<endl;
    // }
    //cout<<"Thread #"<<rankThread <<" read "<<count<<" records "<<results->size()<<endl;

    //delete(dataToUse);
    //////////////////////////////////////////////////////////////
    //                  END COMPUTATION                         //
    //////////////////////////////////////////////////////////////
    
    delete dataToUse->buffer;
    delete dataToUse;

    //COUNTERS
    rc = pthread_mutex_lock(&mutexCounter);
    checkResults("pthread_mutex_lock()\n", rc);

    //cerr<<"Thread #"<<rankThread <<" is done with computations"<<endl;

    results->push_back(statComputer);

    //outputToPrint.push_back(toAddToLog);
    
    //cout<<"Thread #"<<rankThread <<" is re-starting"<<endl;

    rc = pthread_mutex_unlock(&mutexCounter);
    checkResults("pthread_mutex_unlock()\n", rc);


    goto checkqueue;	   


    

    
    //cout<<"Thread "<<rankThread<<" ended "<<endl;
    return NULL;
    
}



template <class STAT> //type 
class parallelP{  

public:
    void launchThreads(string filename,int numberOfThreads,int sizeBins );
};//end class parallelP

//template <class STAT> //type 

// template <typename STAT> 
// vector<STAT> results;

template <class STAT> //type 
void parallelP<STAT>::launchThreads(string filename,int numberOfThreads,int sizeBins ){
    
    doneReading=false;
    //queueFilesToprocess = new queue< vector< string >  * >()  ;
    //queueFilesToprocess = new queue< vector< AlleleRecords >  * >()  ;
    queueFilesToprocess   = new queue< datachunk  * > ();

    //rmd
    // igzstream myFile;
    // myFile.open(filename.c_str(), ios::in);
    // vector<string> lines;

    GlacParser gp (filename);
    vector<string>     chri2chr = gp.getChrKnown();

    if(!gp.isACFormat()){
	cerr<<"GlacCompute: The file "<<filename<<" is not in ACF format"<<endl;
	exit(1);
    }else{
	isGLF=false;
	sizeBytesFormat = int(gp.getSizeOf1DataPoint());
    }
    
    for(unsigned int i=0;i<gp.getPopulationsNames()->size();i++){
	populationNames->push_back(gp.getPopulationsNames()->at(i));
    }

    // vector<string> populationNames;
    //unsigned int numberPopulations;
   
    allowUndefined  = false;
    //populationNames = mp.getPopulationsNames();

    pthread_mutex_init(&mutexTHREADID,   NULL); 
    pthread_mutex_init(&mutexQueue,      NULL); 
    pthread_mutex_init(&mutexCounter,    NULL);

    pthread_t             thread[numberOfThreads];  
    int                   rc=0;   
    vector<STAT * > * results=new vector<STAT *>();
    //cout<<"res "<<results<<endl;
    //launchThreads(numberOfThreads,*thread);
    for(int i=0;i<numberOfThreads;i++){
        rc = pthread_create(&thread[i], NULL, mainComputationThread<STAT>, results); 
        checkResults("pthread_create()\n", rc); 
    }                                          

    //threads are running here  
    //map<string,int> chr2index;//can be obtained from parser
    //map<string,uint16_t> chri2chr;

    //int chr2indexCurrent=0;
    //uint16_t chriLast=UINT16_MAX;

    //    int indexBin;
    //    int lastBin=-1;
    //int chrBin=-1;

    // AlleleRecords  * currentRecord;
    //vector< string  > * vecForBin;
    // vector< AlleleRecords  > * vecForBin;
    //vector< AlleleRecords  > * vecForBin;
       
    //if (myFile.good()){

    //string line;
    //while ( getline (myFile,line)){
    //AlleleRecords * arr;
    
    //while(gp.hasData()){
    //arr = gp.getData();

    //char * buffer = new char [sizeBlock*gp.getSizeRecord()];
    //char * buffer;
    //int sizeRecordsRead=0;

    datachunk * chunkToAdd      = new datachunk;
    uint32_t coordinate=0;
    uint16_t chri=UINT16_MAX;
    try {
	chunkToAdd->buffer          = new char [sizeBins*gp.getSizeRecord()];
    } catch (std::bad_alloc&) {
	cerr<<"Could not allocate a buffer of size "<<(sizeBins*gp.getSizeRecord())<<endl;
	exit(1);
    }

    //cout<<"CREATING BUFFER1 "<<(void*)chunkToAdd->buffer<<endl;
    
    chunkToAdd->sizeRecordsRead = 0;
    // cout<<"chunkToAdd1 "<<chunkToAdd<<endl;
    // cout<<"chunkToAdd buffer #"<<(void *) chunkToAdd->buffer<<"#"<<endl;
    // cout<<"buffer size "<<(sizeBins*gp.getSizeRecord())<<endl;
    // cerr<<"reading block "<<sizeBins<<endl;
    // cout<<"chunkToAdd2 "<<chunkToAdd<<" "<<&chunkToAdd<<endl;

    
    bool rbdReturn = gp.readBlockData(chunkToAdd->buffer,sizeBins,&chunkToAdd->sizeRecordsRead,&chri,&coordinate);
    // cout<<"rbdReturn "<<rbdReturn<<endl;
    // cout<<"chunkToAdd3 "<<chunkToAdd<<" "<<&chunkToAdd<<endl;

    while( rbdReturn ){
	//cout<<*arr<<endl;
	cerr<<"GlacCompute reading new bin, currently  "<<chri2chr[chri]<<":"<<thousandSeparator(coordinate)<<endl;
	
	int rc = pthread_mutex_lock(&mutexQueue);
	checkResults("pthread_mutex_lock()\n", rc);
	
		
	bool needToAskMutex=false;
	//add old
	while( int(queueFilesToprocess->size()) > numberOfThreads ){
	    needToAskMutex=true;
	    //unlock mutex
	    rc = pthread_mutex_unlock(&mutexQueue);
	    checkResults("pthread_mutex_unlock()\n", rc);
	    
	    cerr<<"queue is full, sleeping for 4 seconds"<<endl;
	    sleep(4);
	}

	if(needToAskMutex){
	    rc = pthread_mutex_lock(&mutexQueue);
	    checkResults("pthread_mutex_lock()\n", rc);
	}
		
	queueFilesToprocess->push(chunkToAdd);

	rc = pthread_mutex_unlock(&mutexQueue);
	checkResults("pthread_mutex_unlock()\n", rc);
	
	chunkToAdd                  = new datachunk;
	chunkToAdd->buffer          = new char [sizeBins*gp.getSizeRecord()];
	//cout<<"CREATING BUFFER2 "<<(void*)chunkToAdd->buffer<<endl;
	chunkToAdd->sizeRecordsRead = 0;
	

	//cout<<"p1#"<<arr->chr<<"\t"<<arr->coordinate<<endl;
	//vecForBin->push_back(*arr);
	//vecForBin->push_back(*arr);

	//cout<<"p2#"<<arr->chr<<"\t"<<arr->coordinate<<endl;
	//vecForBin->push_back(line);
	rbdReturn = gp.readBlockData(chunkToAdd->buffer,sizeBins,&chunkToAdd->sizeRecordsRead,&chri,&coordinate);
    }//done reading

    cerr<<"done reading, adding final chunk at "<<chri2chr[chri]<<":"<<thousandSeparator(coordinate)<<endl;
    // cout<<"chunkToAdd "<<chunkToAdd<<endl;
    // cout<<"chri "<<chri<<endl;
    // cout<<"coordinate "<<coordinate<<endl;
    // cout<<"done reading  "<< chunkToAdd->sizeRecordsRead<<endl;


    //adding the last chunk
    rc = pthread_mutex_lock(&mutexQueue);
    checkResults("pthread_mutex_lock()\n", rc);
	
		
    bool needToAskMutex=false;
    //add old
    while( int(queueFilesToprocess->size()) > numberOfThreads ){
	needToAskMutex=true;
	//unlock mutex
	rc = pthread_mutex_unlock(&mutexQueue);
	checkResults("pthread_mutex_unlock()\n", rc);
	    
	cerr<<"queue is full, sleeping for 4 seconds"<<endl;
	sleep(4);
    }

    if(needToAskMutex){
	rc = pthread_mutex_lock(&mutexQueue);
	checkResults("pthread_mutex_lock()\n", rc);
    }
		
    queueFilesToprocess->push(chunkToAdd);

    rc = pthread_mutex_unlock(&mutexQueue);
    checkResults("pthread_mutex_unlock()\n", rc);
	

    //queueFilesToprocess->push(vecForBin);//adding last bin
    //queueFilesToprocess->push(buffer);//adding last bin
    //queueFilesToprocess->push(chunkToAdd);//adding last bin

    //myFile.close();

    // }else{
    // 	cerr << "Unable to open file "<<filename<<endl;
    // 	exit(1);
    // }
    


    doneReading=true;
    //waiting for threads to finish
    for (int i=0; i <numberOfThreads; ++i) {
        rc = pthread_join(thread[i], NULL); 
        checkResults("pthread_join()\n", rc);
    }
    //cout<<"ALL DONE1"<<endl;
    pthread_mutex_destroy(&mutexQueue);   
    pthread_mutex_destroy(&mutexCounter);
    pthread_mutex_destroy(&mutexTHREADID);
    //cout<<"ALL DONE2"<<endl;



    //DO jacknifing
    if(results->size()>1){
	//cout << "VEC "<<vectorToString( *((*results->at(0)).populationNames) )<<endl;
	STAT * allResults=new STAT((*results->at(0)));
	// cout << "VEC "<<vectorToString( *((*results->at(0)).populationNames) )<<endl;
	// cout<<"done all1"<<endl;
	// cout<<allResults->print()<<endl;
	// cout<<"done all2"<<endl;
	vector<STAT * > * jacknife=new vector<STAT *>();

	for (unsigned int i=1; i<results->size() ; ++i) {    
	    //cout<<"add\t"<<i<<endl;	
	    (*allResults)+=(*results->at(i));
	    // //cout<<"done dd1"<<endl;
	    // cout<<allResults->print()<<endl;
	    // cout<<"done dd2"<<endl;
	}

	for (unsigned int i=0; i<results->size() ; ++i) {    
	    // cout<<i<<"\n#####\n"<<endl;
	    //	cout<<results->at(i)<<endl;
	    cout<<"---------------------------"<<endl;
	    cout<<results->at(i)->print()<<endl;
	    //	cout<<i<<"\n#####\n"<<endl;
	}

	for (unsigned int i=0; i<results->size() ; ++i) {    
	    STAT * test =new STAT (*allResults); //creating a copy
	    *test-=(*results->at(i)); //removing ith block
	    jacknife->push_back(test); 
	    //cout<<"ji "<<i<<endl<<test->print()<<endl;
	}	    
	
	//cout<<"done all1"<<endl;
	//COMMENT allResults contains a matrix of AvgCoaResults
	//add a method for jacknife in allResults
	cout<<allResults->printWithBootstraps(jacknife);
	//cout<<allResults->print()<<endl;
	//cout<<"done all2"<<endl;
	
    }else{
	cout<<"GlacCompute: There is only a single bin, cannot perform a jacknife, please run with more data"<<endl;
    }
    pthread_exit(NULL); 
    //cout<<"ALL DONE3"<<endl;
}

string GlacCompute::usage() const{
    string usage=string("glactools")+" compute [options]  <ACF file>"+
                                    "\nThis program computers summary stats on ACF files\n\n"+

	                            "Options:\n"+ 
                                    "\t"+"-p [stats]"  +"\t\t" +"Statistics to use:\n"+
         			      "\t"+"    paircoacompute"+"\tTo compute pairwise average coalescence\n"+
	//			      "\t"+"    fst"+"\t\t\tTo compute pairwise Fst (Weir and Cockerham's 1984)\n"+
			      "\t"+"    dstat"+"\t\tTo compute triple-wise D-statistics\n\n"+



                              "\t"+"-t [threads]"  +"\t\t" +"Threads to use (Default: "+stringify(numberOfThreads)+")\n"+
                              "\t"+"-s [size bin]" +"\t\t" +"Size of bins (Default: "+stringify(sizeBins)+")\n"+
	"\n";


    return usage;
}

int GlacCompute::run(int argc, char *argv[]){

    //int main (int argc, char *argv[]) {


    if(argc == 1 ||
       (argc == 2 && (string(argv[1]) == "-h" || string(argv[1]) == "--help") )
       ){
	cerr << "Usage "<<usage()<<endl;
	return 1;       
    }

    populationNames = new 	vector<string>  ();
    //cout<<"run()"<<endl;
    //cout<<populationNames<<" "<<vectorToString(*populationNames)<<endl;


    for(int i=1;i<(argc-1);i++){ 
	
        if(string(argv[i]) == "-t" ){
            numberOfThreads=destringify<int>(argv[i+1]);
            i++;
            continue;
        }

        if(string(argv[i]) == "-s" ){
	    sizeBins=destringify<int>(argv[i+1]);
            i++;
            continue;
        }

        if(string(argv[i]) == "-p" ){
	    program=string(argv[i+1]);
            i++;
            continue;
        }

        cerr<<"Wrong option "<<argv[i]<<endl;
        return 1;

    }

    if(program == "paircoacompute"){
	parallelP<SumStatAvgCoa> pToRun;
	pToRun.launchThreads(string(argv[argc-1]),numberOfThreads,sizeBins);
    }else{
	if(program == "dstat"){
	    parallelP<SumStatD> pToRun;
	    pToRun.launchThreads(string(argv[argc-1]),numberOfThreads,sizeBins);	
	}else{
	    if(program == "fst"){
		parallelP<SumStatFst> pToRun;
		pToRun.launchThreads(string(argv[argc-1]),numberOfThreads,sizeBins);	
	    }else{
		cerr<<"Wrong program "<<program<<endl;
		return 1;
	    }
	}
    }


	
    return 0;
}



// #ifdef OLD
// 	exit(1);
// 	if( chri !=  chriLast ){  //=UINT16_MAX)
// 	    //chr2index[ chrS ]  = (chr2indexCurrent++);//adding
// 	    chriLast = chri;
// 	    // if(lastBin != -1)
// 	    // 	currentBin = lastBin+1+currentBin;
// 	    if(chrBin == -1){
// 		chrBin = 0;
// 	    }else{
// 		chrBin = lastBin +1; //a step above the last bin
// 	    }
	    
// 	    cerr<<"new chr found, processing chr #: "<<chri2chr[ chri ]<<endl;
// 	    //(chrS)<<endl;
// 	}
	
// 	int currentBin = chrBin+(coordinate/sizeBins);	
// 	cout<<"GlacCompute chri "<<lastBin<<" "<<currentBin<<endl;

// 	if(lastBin != currentBin){
// 	    //cout<<"2: "<<arr->chr<<"\tc="<<arr->coordinate<<"\tbin="<<(currentBin)<<endl;

// 	    //cout<<"new bin"<<endl;
// 	    if( (currentBin%10)==0){
// 		//cerr<<"processing : "<<chrBin<<":"<<coordSUI<<endl;
// 		cerr<<"processing chr #: "<<(chri2chr[ chri ])<<":"<<coordinate<<endl;
// 	    }

// 	    if(lastBin == -1){ //first bin
// 		cout<<"new bin first"<<endl;
// 		//re-init
// 		//vecForBin =  new vector< AlleleRecords > ();				
// 		//vecForBin->reserve(sizeBins);
// 		//buffer = new char [sizeBlock*gp.getSizeRecord()];
// 		datachunk * chunkToAdd      = new datachunk;
// 		chunkToAdd->buffer          = new char [sizeBins*gp.getSizeRecord()];
// 		cout<<"CREATING BUFFER2 "<<(void*)chunkToAdd->buffer<<endl;
// 		chunkToAdd->sizeRecordsRead = 0;
		
// 		//vecForBin =  new vector< string > ();		

// 		lastBin=currentBin;
// 	    }else{

// 		int rc = pthread_mutex_lock(&mutexQueue);
// 		checkResults("pthread_mutex_lock()\n", rc);
// 		//cout<<"new bin old\t"<<int(queueFilesToprocess->size())<<"\tresadr\t"<<results<<"\tressize\t"<<results->size()<<endl;
		
// 		bool needToAskMutex=false;
// 		//add old
// 		while( int(queueFilesToprocess->size()) > numberOfThreads ){
// 		    needToAskMutex=true;
// 		    //unlock mutex
// 		    rc = pthread_mutex_unlock(&mutexQueue);
// 		    checkResults("pthread_mutex_unlock()\n", rc);

// 		    //cout<<"Queue is full main threads will sleep for 10 seconds and wait for threads to finish"<<endl;
// 		    sleep(4);
// 		}

// 		if(needToAskMutex){
// 		    rc = pthread_mutex_lock(&mutexQueue);
// 		    checkResults("pthread_mutex_lock()\n", rc);
// 		}
		
// 		//queueFilesToprocess->push(vecForBin);
// 		//buffer = new char [sizeBlock*gp.getSizeRecord()];
// 		queueFilesToprocess->push(chunkToAdd);

// 		rc = pthread_mutex_unlock(&mutexQueue);
// 		checkResults("pthread_mutex_unlock()\n", rc);
		
// 		//cout<<"pushing "<<vecForBin<<endl;
// 		// for(unsigned int j=0;j<vecForBin->size();j++){
// 		//     cout<<"v "<<(vecForBin->at(j))<<endl;
// 		// }
// 		//re-init
// 		//vecForBin =  new vector< string  > ();		
// 		//vecForBin =  new vector< AlleleRecords  > ();
// 		//vecForBin->reserve(sizeBins);
// 		//buffer = new char [sizeBlock*gp.getSizeRecord()];
// 		datachunk * chunkToAdd      = new datachunk;
// 		chunkToAdd->buffer          = new char [sizeBins*gp.getSizeRecord()];
// 		cout<<"CREATING BUFFER3 "<<(void*)chunkToAdd->buffer<<endl;
// 		chunkToAdd->sizeRecordsRead = 0;


// 		lastBin=currentBin;
// 		//
// 	    }
// 	}else{
// 	    //cout<<"old bin"<<endl;
// 	}
// #endif	
