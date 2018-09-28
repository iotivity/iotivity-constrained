package java_lite_simple_client;

import java.util.Arrays;

import org.iotivity.DiscoveryHandler;
import org.iotivity.OCDiscoveryFlags;
import org.iotivity.OCEndpoint;
import org.iotivity.OCInterfaceMask;
import org.iotivity.OCMain;
import org.iotivity.OCQos;
import org.iotivity.OCResourcePropertiesMask;
import org.iotivity.OCTransportFlags;

public class MyDiscoveryHandler implements DiscoveryHandler {

    @Override
    public OCDiscoveryFlags handler(String anchor, String uri, String[] types, int interfaceMask, OCEndpoint endpoint,
                                    int resourcePropertiesMask, Object userData) {
        System.out.println("DiscoveryHandler");
        System.out.println("\tanchor: " + anchor);
        System.out.println("\turi: " + uri);
        System.out.println("\ttypes: " + Arrays.toString(types));

        String interfaces_found = "";
        if ((interfaceMask & OCInterfaceMask.S) > 0) {
            interfaces_found += "S";
        }
        if ((interfaceMask & OCInterfaceMask.A) > 0) {
            if(!interfaces_found.isEmpty()) {
                interfaces_found += " | ";
            }
            interfaces_found += "A";
        }
        if ((interfaceMask & OCInterfaceMask.RW) > 0) {
            if(!interfaces_found.isEmpty()) {
                interfaces_found += " | ";
            }
            interfaces_found += "RW";
        }
        if ((interfaceMask & OCInterfaceMask.R) > 0) {
            if(!interfaces_found.isEmpty()) {
                interfaces_found += " | ";
            }
            interfaces_found += "R";
        }
        if ((interfaceMask & OCInterfaceMask.B) > 0) {
            if(!interfaces_found.isEmpty()) {
                interfaces_found += " | ";
            }
            interfaces_found += "B";
        }
        if ((interfaceMask & OCInterfaceMask.LL) > 0) {
            if(!interfaces_found.isEmpty()) {
                interfaces_found += " | ";
            }
            interfaces_found += "LL";
        }
        if ((interfaceMask & OCInterfaceMask.BASELINE) > 0) {
            if(!interfaces_found.isEmpty()) {
                interfaces_found += " | ";
            }
            interfaces_found += "BASELINE";
        }
        System.out.println("\tinterfaces: " + interfaces_found);

        String properties_found = "";
        if ((resourcePropertiesMask & OCResourcePropertiesMask.OC_PERIODIC) > 0) {
            properties_found += "PERIODIC";
        }
        if ((resourcePropertiesMask & OCResourcePropertiesMask.OC_SECURE) > 0) {
            if(!properties_found.isEmpty()) {
                properties_found += " | ";
            }
            properties_found += "SECURE";
        }
        if ((resourcePropertiesMask & OCResourcePropertiesMask.OC_OBSERVABLE) > 0) {
            if(!properties_found.isEmpty()) {
                properties_found += " | ";
            }
            properties_found += "OBSERVABLE";
        }
        if ((resourcePropertiesMask & OCResourcePropertiesMask.OC_DISCOVERABLE) > 0) {
            if(!properties_found.isEmpty()) {
                properties_found += " | ";
            }
            properties_found += "DISCOVERABLE";
        }
        System.out.println("\tresource properties: " + properties_found);

        for (String type: types) {
            if(type.equals("core.light")) {
                Light.server = endpoint;
                Light.server_uri = uri;
                System.out.println("\tResource " + Light.server_uri + " hosted at endpoints:");
                System.out.println("\t\tendpoint.device " + endpoint.getDevice());
                System.out.println("\t\tendpoint.flags " + endpoint.getFlags());
                System.out.println("\t\tendpoint.interfaceIndex " + endpoint.getInterfaceIndex());
                System.out.println("\t\tendpoint.version " + endpoint.getVersion().toString());
                OCEndpoint ep = endpoint;
                while (ep != null) {
                    if ((ep.getFlags() & OCTransportFlags.IPV4) > 0) {
                        System.out.println("\t\tendpoint.IPv4Address [" + ep.getAddr().getIpv4().getAddress()[0] + "." +
                                ep.getAddr().getIpv4().getAddress()[1] + "." +
                                ep.getAddr().getIpv4().getAddress()[2] + "." +
                                ep.getAddr().getIpv4().getAddress()[3] + "]:" +
                                ep.getAddr().getIpv4().getPort());
                    } else if ((ep.getFlags() & OCTransportFlags.IPV6) > 0) {
                        System.out.println(String.format("\t\tendpoint.IPv6Address [%02x%02x:%02x%02x:%02x%02x:%02x%02x:"
                                                       + "%02x%02x:%02x%02x:%02x%02x:%02x%02x]:%d",
                                                       ep.getAddr().getIpv6().getAddress()[0],
                                                       ep.getAddr().getIpv6().getAddress()[1],
                                                       ep.getAddr().getIpv6().getAddress()[2],
                                                       ep.getAddr().getIpv6().getAddress()[3],
                                                       ep.getAddr().getIpv6().getAddress()[4],
                                                       ep.getAddr().getIpv6().getAddress()[5],
                                                       ep.getAddr().getIpv6().getAddress()[6],
                                                       ep.getAddr().getIpv6().getAddress()[7],
                                                       ep.getAddr().getIpv6().getAddress()[8],
                                                       ep.getAddr().getIpv6().getAddress()[9],
                                                       ep.getAddr().getIpv6().getAddress()[10],
                                                       ep.getAddr().getIpv6().getAddress()[11],
                                                       ep.getAddr().getIpv6().getAddress()[12],
                                                       ep.getAddr().getIpv6().getAddress()[13],
                                                       ep.getAddr().getIpv6().getAddress()[14],
                                                       ep.getAddr().getIpv6().getAddress()[15],
                                                       ep.getAddr().getIpv6().getPort()
                                                       ));
                    }
                    ep = ep.getNext();
                }
                GetLightResponseHandler responseHandler = new GetLightResponseHandler();
                OCMain.doGet(Light.server_uri, Light.server, null, OCQos.LOW_QOS, responseHandler);
                return OCDiscoveryFlags.OC_STOP_DISCOVERY;
            }
        }
        OCMain.freeServerEndpoints(endpoint);
        return OCDiscoveryFlags.OC_CONTINUE_DISCOVERY;
    }
}
