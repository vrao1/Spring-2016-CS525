import java.io.*;
import java.util.*;

public class SQLProcessor{
    
    public int totalTuples;
    public int totalColumns;

    SQLInput obj;

    public SQLProcessor(){
	totalTuples = 0;
	totalColumns = 0;
    }

    public SQLProcessor(SQLInput inputObj){
            obj = inputObj;
    }

    public int doProcessing()
    {    
    
    String fileName = obj.getFileName();
    if(getNumberColumnsRows()<0) return -1;
    
    String[] buffer = new String[obj.getBufferSize()];

    try{
    	FileReader in = new FileReader(fileName);

    BufferedReader br = new BufferedReader(in);
    
    String row;
    String delim = ",";
    
//    for(int i=0;i<obj.getBufferSize();i++)
 //   buffer[i] = new String();
    
    switch(obj.getOperation()){
        case 1:
		System.out.println("This Selection will be executed by Tuple-by-Tuple");
    		executeSelect(buffer);
		break;
        case 2:
		System.out.println("This Projection will be executed by Tuple-by-Tuple");
    		executeProjection(buffer);
		break;
        case 3:
		System.out.println("This Duplicate Removal will be executed by All Tuples in memory at once");
		if(totalTuples <= obj.getBufferSize())
    			executeDistinct(buffer);
		else
			System.out.println("The Memory Buffer Length is shorter than the number of tuples in the data file,Duplicate Removal can't be possible without External Algorithm");
		break;
        case 4:
        case 5:
        case 6:
        case 7:
		System.out.println("This Aggregate operation will be executed by All Tuples in memory at once");
		if(totalTuples <= obj.getBufferSize())
    			executeAggregate(buffer);
		else
			System.out.println("The Memory Buffer Length is shorter than the number of tuples in the data file,Aggregate operation can't be possible without External Algorithm");
		break;
	default:
		System.out.println("ERROR: Wrong Input");
    }
    }catch(FileNotFoundException fnt){
	System.out.println("ERROR: File Not Found");
    }		
    return 1;	
}

public int getNumberColumnsRows(){
       
    try{
    	FileReader in = new FileReader(obj.getFileName());
		
    BufferedReader br = new BufferedReader(in);
    
    String row;
    String delim = ",";
    
    while((row = br.readLine()) != null){
        //row = br.readLine();
        StringTokenizer st = new StringTokenizer(row,delim);
        if(totalColumns != 0){
            if(totalColumns != st.countTokens()){
                System.out.println("Number of columns do not match with different rows");
                return -1;
            }
        }
        totalColumns = st.countTokens();
        totalTuples++;
        st = null;
    }
    
    in.close();
    br = null;
    }catch(IOException ioe){
	System.out.println("ERROR : IO Exception");
    }/*catch(...){
	System.out.println("ERROR: File Not Found");
    }*/
    return 1;
}

//Execute Select Operation
public int executeSelect(String[] buffer){
       
    try{
    	FileReader in = new FileReader(obj.getFileName());
    BufferedReader br = new BufferedReader(in);
    
    String delim = ",";
    int column = obj.getColumn();
    String value = obj.getValue();
    int j = 0;
	
    //Reading File
    while((buffer[0] = br.readLine()) != null){
	String tmp = buffer[0];
	if(column == 0){
		System.out.println(buffer[0]);
	}else{
        	int i = 1;	
		for(String retval: tmp.split(delim)){
			if(i == column && retval.equals(value)){
				System.out.println(buffer[0]);
				break;
			}
			i++;
		}
	}
    }
    
    in.close();
    br = null;
    }catch(IOException ioe){
	System.out.println("ERROR : IO Exception");
    }/*catch(FileNotFoundException fnt){
	System.out.println("ERROR: File Not Found");
    }*/
    
    return 1;
}

//Execute Projection Operation
public int executeProjection(String[] buffer){
       
    try{
    	FileReader in = new FileReader(obj.getFileName());
    BufferedReader br = new BufferedReader(in);
    
    String delim = ",";
    int column = obj.getColumn();
   int j=0; 
    //Reading File
    while((buffer[0] = br.readLine()) != null){
	String tmp = buffer[0];
	if(column == 0){
		System.out.println(buffer[0]);
	}else{
        	int i = 1;	
		for(String retval: tmp.split(delim)){
			if(i == column){
				System.out.println(retval);
				break;
			}
			i++;
		}
	}
    }
    
    in.close();
    br = null;

    }catch(IOException ioe){
	System.out.println("ERROR : IO Exception");
    }/*catch(FileNotFoundException fnt){
	System.out.println("ERROR: File Not Found");
    }*/
    return 1;
}

//Execute Duplicate Removal Operation
public int executeDistinct(String[] buffer){
       
    try{
    	FileReader in = new FileReader(obj.getFileName());
    
    BufferedReader br = new BufferedReader(in);
    HashMap<String,String> hmap = new HashMap<String,String>();
 
    String delim = ",";
    int column = obj.getColumn();
    int total_Tuples = 0;
    int i = 0;
     	
    //Reading File
    while((buffer[total_Tuples] = br.readLine()) != null){
        total_Tuples++;
    }
    		
    while(i < total_Tuples){
	String tmp = buffer[i];

	if(column == 0){
		hmap.put(buffer[i],buffer[i]);
	}else{
        	int j = 1;	
		for(String retval: tmp.split(delim)){
			if(j == column){
				hmap.put(retval,buffer[i]);
				break;
			}
			j++;
		}
	}
	i++;
    }
    
    in.close();
    br = null;

    Iterator<String> itr = hmap.keySet().iterator();
    while(itr.hasNext()){
    	String key = itr.next();
	System.out.println(hmap.get(key));
    }	
    
    hmap.clear();			
    }catch(IOException ioe){
	System.out.println("ERROR : IO Exception");
    }/*catch(FileNotFoundException fnt){
	System.out.println("ERROR: File Not Found");
    }*/
    return 1;
}

//Execute Aggregate Operation like MIN,MAX,SUM,AVG
public int executeAggregate(String[] buffer){
       
    try{
    	FileReader in = new FileReader(obj.getFileName());
    
    BufferedReader br = new BufferedReader(in);
    int whichOperation = obj.getOperation();
			


    HashMap<String,Integer> hmap = new HashMap<String,Integer>();
    HashMap<String,Integer> hmapCounter = new HashMap<String,Integer>();
 
    String delim = ",";
    int column = obj.getColumn();
    int groupOn = obj.getGroupOn();
    int total_Tuples = 0;
    int i = 0;

    int minVal=99999; 	
    int maxVal=-99999;
    int tokenVal;
	
    int sum = 0;
    ArrayList<String> tokens = new ArrayList<String>();
    
    //Reading File
    while((buffer[total_Tuples] = br.readLine()) != null){
        total_Tuples++;
    }
    		
    while(i < total_Tuples){
	String tmp = buffer[i];
	int j=0;
	for(String retval: tmp.split(delim)){
		tokens.add(retval);
	}

	tokenVal = Integer.parseInt(tokens.get(column-1));

    	switch(whichOperation){
		case 4:
			if(groupOn == 0){
    				if(minVal > tokenVal)
    				minVal = tokenVal;
			}else{
				if(hmap.containsKey(tokens.get(groupOn-1))){
					if(hmap.get(tokens.get(groupOn-1)) > tokenVal)
					hmap.put(tokens.get(groupOn-1),tokenVal);
				}else{
					hmap.put(tokens.get(groupOn-1),tokenVal);
				}
			}
			break;
		case 5:
			if(groupOn == 0){
    				if(maxVal < tokenVal)
    				maxVal = tokenVal;
			}else{
				if(hmap.containsKey(tokens.get(groupOn-1))){
					if(hmap.get(tokens.get(groupOn-1)) < tokenVal)
					hmap.put(tokens.get(groupOn-1),tokenVal);
				}else{
					hmap.put(tokens.get(groupOn-1),tokenVal);
				}
			}
			break;
		case 6:
			if(groupOn == 0){
    				sum += tokenVal;
			}else{
				if(hmap.containsKey(tokens.get(groupOn-1))){
					int val = hmap.get(tokens.get(groupOn-1));
					hmap.put(tokens.get(groupOn-1),val+tokenVal);
				}else
					hmap.put(tokens.get(groupOn-1),tokenVal);
			}
			break;
		case 7:
			if(groupOn == 0){
    				sum += tokenVal;
			}else{
				if(hmap.containsKey(tokens.get(groupOn-1))){
					int val = hmap.get(tokens.get(groupOn-1));
					hmap.put(tokens.get(groupOn-1),val+tokenVal);
					int v = hmapCounter.get(tokens.get(groupOn-1));
					hmapCounter.put(tokens.get(groupOn-1),v+1);
				}else{
					hmap.put(tokens.get(groupOn-1),tokenVal);
					hmapCounter.put(tokens.get(groupOn-1),1);
				}
			}
			break;
		default:
			System.out.println("ERROR");
			break;
    	}
	i++;
	tokens.clear();
    }
    
    in.close();
    br = null;

    Iterator<String> itr = hmap.keySet().iterator();
    int average;

    	switch(whichOperation){
		case 4:
			if(groupOn == 0){
				System.out.println(minVal);
			}else{
    				while(itr.hasNext()){
    				String key = itr.next();
				System.out.println(key+"\t"+hmap.get(key));
    				}	
			}
			break;
		case 5:
			if(groupOn == 0){
				System.out.println(maxVal);
			}else{
    				while(itr.hasNext()){
    				String key = itr.next();
				System.out.println(key+"\t"+hmap.get(key));
    				}	
			}
			break;
		case 6:
			if(groupOn == 0){
				System.out.println(sum);
			}else{
    				while(itr.hasNext()){
    				String key = itr.next();
				System.out.println(key+"\t"+hmap.get(key));
    				}	
			}
			break;
		case 7:
			if(groupOn == 0){
				average = sum / total_Tuples;
				System.out.println(average);
			}else{
    				//Iterator<String> itrC = hmapCounter.keySet().iterator();
    				while(itr.hasNext()){
    				String key = itr.next();
				average = hmap.get(key) / hmapCounter.get(key); 
				System.out.println(key+"\t"+average);
    				}	
			}
			break;
		default:
			System.out.println("ERROR");
			break;
    	}

    
    hmap.clear();			
    hmapCounter.clear();
    }catch(IOException ioe){
	System.out.println("ERROR : IO Exception");
    }/*catch(FileNotFoundException fnt){
	System.out.println("ERROR: File Not Found");
    }*/
    return 1;
}
}
