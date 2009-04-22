import java.io.File;
import org.w3c.dom.Document;
import org.w3c.dom.*;

import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.DocumentBuilder;
import org.xml.sax.SAXException;
import org.xml.sax.SAXParseException; 

/** 
 * Parses an XML file containing a VM description created by OpenNebula Core
 **/ 

public class ParseXML
{

    private String   cpu;
    private String[] disk;
    private String   memory;
    private String[] macs;
    
    /**
     * Parses the XML file and fills the values
     * @param fileName full path of the file to be parsed
     **/
    ParseXML(String fileName) throws Exception
    {
        try 
        {            
            DocumentBuilderFactory docBuilderFactory = DocumentBuilderFactory.newInstance();
            DocumentBuilder docBuilder = docBuilderFactory.newDocumentBuilder();
            
            Document doc = docBuilder.parse (new File(fileName));
            
            doc.getDocumentElement().normalize();

            NodeList vm = doc.getElementsByTagName("VM");
            
            if(vm.getLength()!=1)
            {
                throw new Exception("Number of VM tags different of 1: [" + vm.getLength() + "]");
            }
            
            // CPU
            
            NodeList cpuNL = doc.getElementsByTagName("CPU");
            
            if(cpuNL.getLength()!=1)
            {
                throw new Exception("Number of CPU tags different of 1: [" + cpuNL.getLength() + "]");
            }
            
            cpu     = ((Node)cpuNL.item(0)).getNodeValue().trim();
            
            // Memory
            
            NodeList memoryNL = doc.getElementsByTagName("MEMORY");
            
            if(memoryNL.getLength()!=1)
            {
                throw new Exception("Number of MEMORY tags different of 1: [" + memoryNL.getLength() + "]");
            }
            
            memory   = ((Node)memoryNL.item(0)).getNodeValue().trim();
            
            // DISK
            
            NodeList diskNL = doc.getElementsByTagName("DISK");
            
            if(diskNL.getLength()!=0)
            {
                disk = new String[diskNL.getLength()];
                
                for(int i=0; i<diskNL.getLength(); i++)
                {
                    NodeList sourceNode = ((Element)diskNL).getElementsByTagName("SOURCE");
                    
                    disk[i] = ((Node)sourceNode.item(0)).getNodeValue().trim();
                }
            }
            
            // Network
            
            NodeList nwNL = doc.getElementsByTagName("NIC");
            
            if(nwNL.getLength()!=0)
            {
                macs = new String[nwNL.getLength()];
                
                for(int i=0; i<nwNL.getLength(); i++)
                {
                    NodeList mac = ((Element)nwNL).getElementsByTagName("MAC");
                    
                    macs[i] = ((Node)mac.item(0)).getNodeValue().trim();
                }
            }            
        }
        catch (SAXParseException err) 
        {
            throw new Exception("** Parsing error" + ", line " 
                 + err.getLineNumber () + ", uri " + err.getSystemId ());
        }
    }// end ParseXML

    /**
     * Returns cpu value
     * @return cpu number of cpus to be used by the VM
     **/    
    String getCPU()
    {
        return cpu;
    }
    
    /**
     * Returns disk value
     * @return cpu array with the full local path of disks
     **/
    String[] getDisk()
    {
        return disk;
    }
    
    /**
     * Returns memory value
     * @return memory amount of memory in Kb to be used by this VM
     **/    
    String getMemory()
    {
        return memory;
    }
    
    /**
     * Returns networks MACs
     * @return macs array with the macs of the NICs to be added to this VM
     **/
    String[] getNet()
    {
        return macs;
    }
}