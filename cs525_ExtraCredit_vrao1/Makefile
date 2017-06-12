JFLAGS = -g
JC = javac
.SUFFIXES: .java .class
.java.class:
	$(JC) $(JFLAGS) $*.java

CLASSES = \
        SQLInput.java \
        SQLProcessor.java \
        SQLManager.java

default: classes

classes: $(CLASSES:.java=.class)

clean:
	$(RM) *.class
