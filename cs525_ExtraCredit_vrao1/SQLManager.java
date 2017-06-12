import java.io.*;

public class SQLManager{
    public static void main(String []args){
	// Validate Passed Command Line Argument
        doValidateArguments(args);
	String t = "n";
	try{
	BufferedReader br = new BufferedReader(new InputStreamReader(System.in));

	while(t.equals("n")){

        SQLInput obj = new SQLInput(args[0]);

	// Quit Input will be asked in getUserInput
        if(obj.getUserInput() < 0){
		obj = null;
		System.exit(0);
	}

        SQLProcessor procObj = new SQLProcessor(obj);
        procObj.doProcessing();
	obj = null;
	procObj.obj = null;
	procObj = null;
	
	System.out.println("Would You like to quit this process (y/n)?");
        t = br.readLine();

	}

	br = null;
	}catch(IOException ioe){
		System.out.println("ERROR : IOException");
	}
    }

    public static void doValidateArguments(String []args){
    	if(args.length != 1){
		System.out.println("USAGE : java SQLManager <INPUT DATA FILE>");
		System.exit(0);
	} 
    }		
}
