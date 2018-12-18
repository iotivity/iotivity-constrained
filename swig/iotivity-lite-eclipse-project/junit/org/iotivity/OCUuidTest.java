package org.iotivity;

import static org.junit.Assert.*;

import org.junit.Test;

public class OCUuidTest {

    @Test
    public void generate_and_convert() {
        OCUuid testUuid = OCUuidUtil.generateUuid();
        assertNotNull(testUuid);
        String uuid_out = OCUuidUtil.uuidToString(testUuid);
        assertEquals('-', uuid_out.charAt(8));
        assertEquals('-', uuid_out.charAt(13));
        assertEquals('-', uuid_out.charAt(18));
        assertEquals('-', uuid_out.charAt(23));
        // OC_UUID_LEN is one character longer than length for the '\0' nul terminating
        // character in C
        assertEquals(OCUuidUtil.OC_UUID_LEN-1, uuid_out.length());
    }
    
    @Test
    public void convert_string_to_uuid() {
        // Random version 4 uuid, generated using www.uuidgenerator.net
        String generated_uuid = "a4fba108-877c-469e-9270-b400839b0631";
        OCUuid testUuid = OCUuidUtil.stringToUuid(generated_uuid);
        assertEquals(generated_uuid, OCUuidUtil.uuidToString(testUuid));
    }

    @Test
    public void convert_invalid_string_to_uuid() {
        // Random version 4 uuid, generated using www.uuidgenerator.net with
        // invalid letters inserted to make it an invalid uuid.
        String generated_uuid = "xxxxa108-877cx469e-9270-b400839b0631";
        // stringToUuid has no error checking just verifying the function will not crash
        OCUuid testUuid = OCUuidUtil.stringToUuid(generated_uuid);
        assertNotEquals(generated_uuid, OCUuidUtil.uuidToString(testUuid));

        String long_invalid_uuid = "i-wish-i-had-a-dollar-for-every-bug-i-created-"
                + "and-fixed-then-i-would-be-a-wealthy-person";
        testUuid = OCUuidUtil.stringToUuid(long_invalid_uuid);
        assertNotEquals(long_invalid_uuid, OCUuidUtil.uuidToString(testUuid));
    }

}
