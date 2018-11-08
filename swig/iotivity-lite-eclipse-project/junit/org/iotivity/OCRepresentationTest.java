package org.iotivity;

import static org.junit.Assert.*;

import org.iotivity.OCRepresentation.OCValue;
import org.junit.Test;

public class OCRepresentationTest {
    @Test
    public void testType() {
        OCRepresentation req = new OCRepresentation();
        assertNotNull(req);
        req.setType(OCType.OC_REP_INT);
        assertEquals(OCType.OC_REP_INT, req.getType());
    }

    @Test
    public void testNext() {
        OCRepresentation req1 = new OCRepresentation();
        OCRepresentation req2 = new OCRepresentation();
        OCRepresentation req3 = new OCRepresentation();
        assertNotNull(req1);
        assertNotNull(req2);
        assertNotNull(req3);
        req1.setName("one");
        req2.setName("two");
        req3.setName("three");
        req1.setNext(req2);
        req1.getNext().setNext(req3);
        OCRepresentation r = req1;
        assertEquals("one", r.getName());
        r = r.getNext();
        assertNotNull(r);
        assertEquals("two", r.getName());
        r = r.getNext();
        assertNotNull(r);
        assertEquals("three", r.getName());
        r = r.getNext();
        assertNull(r);
    }

    @Test
    public void testName() {
        OCRepresentation req = new OCRepresentation();
        assertNotNull(req);
        req.setName("Sam");
        assertEquals("Sam", req.getName());
    }

    @Test
    public void testValue() {
        OCRepresentation req = new OCRepresentation();
        assertNotNull(req);
        OCValue v = new OCValue();
        v.setString("happy dog");
        req.setValue(v);
        assertEquals("happy dog", req.getValue().getString());
    }
    
    @Test
    public void testValueInteger() {
        OCValue v = new OCValue();
        assertNotNull(v);
        v.setInteger(1010101);
        assertEquals(1010101, v.getInteger());
        v.setInteger(42);
        assertEquals(42, v.getInteger());
        v.setInteger(0);
        assertEquals(0, v.getInteger());
        v.setInteger(-54);
        assertEquals(-54, v.getInteger());
    }

    @Test
    public void testValueBool() {
        OCValue v = new OCValue();
        assertNotNull(v);
        v.setBool(true);
        assertTrue(v.getBool());
        v.setBool(false);
        assertFalse(v.getBool());
    }

    @Test
    public void testValueDouble() {
        OCValue v = new OCValue();
        assertNotNull(v);
        v.setDouble(3.14159265359);
        assertEquals(3.14159265359, v.getDouble(), 0.0000000001);
        v.setDouble(1.618033988749895);
        assertEquals(1.618033988749895, v.getDouble(), 0.000000000000001);
        v.setDouble(-1.618033988749895);
        assertEquals(-1.618033988749895, v.getDouble(), 0.00000000000001);
        v.setDouble(-3.14159265359);
        assertEquals(-3.14159265359, v.getDouble(), 0.0000000001);
    }

    @Test
    public void testValueString() {
        OCValue v = new OCValue();
        assertNotNull(v);
        v.setString("more");
        assertEquals("more", v.getString());
        v.setString("");
        assertEquals("", v.getString());
    }

    @Test
    public void testValueArray() {
        fail("Not yet implemented");
    }

    @Test
    public void testValueObject() {
        fail("Not yet implemented");
    }

    @Test
    public void testValueObjectArray() {
        fail("Not yet implemented");
    }

    @Test
    public void testRepInt() {
        byte buffer[] = new byte[1024];
        OCMain.repNew(buffer);
        CborEncoder root = OCMain.repBeginRootObject();
        assertNotNull(root);
        OCMain.repSetInt(root, "ultimat_answer", 42);
        OCMain.repEndRootObject();
        OCMain.repSetPool(new OCMemoryBuffer());
        OCRepresentation rep = OCMain.repGetOCRepresentaionFromRootObject();
        assertNotNull(rep);
        int[] outValue = new int[1];
        OCMain.repGetInt(rep, "ultimat_answer", outValue);
        assertEquals(42, outValue[0]);
    }

    @Test
    public void testRepBool() {
        byte buffer[] = new byte[1024];
        OCMain.repNew(buffer);
        CborEncoder root = OCMain.repBeginRootObject();
        assertNotNull(root);
        OCMain.repSetBoolean(root, "true_flag", true);
        OCMain.repEndRootObject();
        OCMain.repSetPool(new OCMemoryBuffer());
        OCRepresentation rep = OCMain.repGetOCRepresentaionFromRootObject();
        assertNotNull(rep);
        boolean[] outValue = new boolean[1];
        OCMain.repGetBool(rep, "true_flag", outValue);
        assertEquals(true, outValue[0]);
    }

    @Test
    public void testRepDouble() {
        byte buffer[] = new byte[1024];
        OCMain.repNew(buffer);
        CborEncoder root = OCMain.repBeginRootObject();
        assertNotNull(root);
        OCMain.repSetDouble(root, "pi", 3.14);
        OCMain.repEndRootObject();
        OCMain.repSetPool(new OCMemoryBuffer());
        OCRepresentation rep = OCMain.repGetOCRepresentaionFromRootObject();
        assertNotNull(rep);
        double[] outValue = new double[1];
        OCMain.repGetDouble(rep, "pi", outValue);
        assertEquals(3.14, outValue[0], 0.001);
    }

    @Test
    public void testRepString() {
        byte buffer[] = new byte[1024];
        OCMain.repNew(buffer);
        CborEncoder root = OCMain.repBeginRootObject();
        assertNotNull(root);
        OCMain.repSetTextString(root, "hello", "world");
        OCMain.repEndRootObject();
        OCMain.repSetPool(new OCMemoryBuffer());
        OCRepresentation rep = OCMain.repGetOCRepresentaionFromRootObject();
        assertNotNull(rep);
        String[] outValue = new String[1];
        OCMain.repGetString(rep, "hello", outValue);
        assertEquals("world", outValue[0]);
    }
}
