import java.io.*;
import java.lang.*;

public class SQLInput{
    
        int whichOperation;
        int column;
        String value;
        int groupingOn;
        int bufferSize;
        String fileName;
   
        public SQLInput(String filename){
            this.fileName = filename;
        }
        
        public int getOperation(){
            return whichOperation;
        }
        
        public int getColumn(){
            return column;
        }
        
        public String getValue(){
            return value;
        }
        
        public int getGroupOn(){
            return groupingOn;
        }
        
        public int getBufferSize(){
            return bufferSize;
        }
        
        public String getFileName(){
            return fileName;
        }


public int getUserInput(){
    
    BufferedReader br = new BufferedReader(new InputStreamReader(System.in));
    System.out.println("How many memory locations would you like to allocate ? ");
try{
    bufferSize = Integer.parseInt(br.readLine());
    if(bufferSize < 1) {
    	System.out.println("Buffer Size must be greater than 0 ");
	return 1;
    }	
    System.out.println("Which Operation would you like to do ? ");
    System.out.println("\t\t\tFor Select type 1 : ");
    System.out.println("\t\t\tFor Projection type 2 : ");
    System.out.println("\t\t\tFor Duplicate Removal type 3 : ");
    System.out.println("\t\t\tFor Aggregate Function MIN type 4 : ");
    System.out.println("\t\t\tFor Aggregate Function MAX type 5 : ");
    System.out.println("\t\t\tFor Aggregate Function SUM type 6 : ");
    System.out.println("\t\t\tFor Aggregate Function AVG type 7 : ");
    System.out.print("\t\t\t : ");
    
    
    
    whichOperation = Integer.parseInt(br.readLine());
    
    switch(whichOperation){
        case 1:
                System.out.print("\t\t\tFor particular column and it's value type  following \n\t\t\t<Column Number> <Enter>\n\t\t\t<Column Value> <Enter>\n\t\t\t (Type 0 for all and Hit <Enter> <Enter> two times)\n\t\t\t ");
                column = Integer.parseInt(br.readLine());
                value = br.readLine();
                break;
        case 2:
                System.out.println("\t\t\tWhich Column do you want to project : (Type 0 for all columns) ");
                column = Integer.parseInt(br.readLine());
                break;
        case 3:
                System.out.println("\t\t\tOn which Column do you want to remove duplicate : (Type 0 for all columns) ");
                column = Integer.parseInt(br.readLine());
                break;
        case 4:
        case 5:
        case 6:
        case 7:
                System.out.println("\t\t\tOn which column do you want to execute this aggregate operation : ");
                column = Integer.parseInt(br.readLine());
                System.out.println("\t\t\tOn which column do you want to group by to execute this aggregate operation : (Type 0 for all rest columns)");
                groupingOn = Integer.parseInt(br.readLine());
                break;
        default:
                System.out.println("Invalid input by user\n\n\n");
		return -1;
    }
}catch(IOException ioe){
	System.out.println("ERROR: Can't Read from Console !");
}
	return 0;
}
}
