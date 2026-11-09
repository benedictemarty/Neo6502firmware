// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      group40_database.inc
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      9th November 2026
//      Purpose :   Toolbox group 40, Database (T-138)
//
// ***************************************************************************************
// ***************************************************************************************

GROUP 40 Database
    FUNCTION 1 Create Database
        *DERROR = DBCreate(DPARAMS[0],DSPGetInt16(DPARAMS,1),DSPGetInt16(DPARAMS,3));
    DOCUMENTATION
        Toolbox (Trinity, T-138). Creates, or empties if it exists, the database file whose length-prefixed name is at
        Parameters:1,2, with records of Parameters:3,4 bytes (2 to 4096), using file channel Parameter:0 for the call
        only (the file is closed afterwards ; open it with Function 40,2). ND1 file : 'N','D',1,0, record size (16 bit),
        record count (32 bit), 6 reserved bytes, then the records. Byte 0 of each record is its status : 1 active,
        0 deleted ; the rest is yours. Errors : 1 bad parameter, 2 storage error.

    FUNCTION 2 Open Database
        *DERROR = DBOpen(DPARAMS[0],DSPGetInt16(DPARAMS,1));
    DOCUMENTATION
        Opens for reading and writing, on file channel Parameter:0, the database whose length-prefixed name is at
        Parameters:1,2. One database is open at a time (opening another closes the first). Nothing is cached in the
        RP2040 but the channel, the record size and the count : the data stays on the storage. Errors : 1 bad channel
        or not a database (or more than 65535 records), 2 storage error. Records are numbered 1..count.

    FUNCTION 3 Close Database
        *DERROR = DBClose();
    DOCUMENTATION
        Closes the database and its channel (error 1 if none is open).

    FUNCTION 4 Database Info
        {
            uint16_t count = 0,size = 0;
            *DERROR = DBInfo(&count,&size);
            DSPSetInt16(DPARAMS,0,count);DSPSetInt16(DPARAMS,2,size);
        }
    DOCUMENTATION
        Returns in Parameters:0,1 the number of records (deleted ones included) and in Parameters:2,3 the record size.

    FUNCTION 5 Read Record
        *DERROR = DBRead(DSPGetInt16(DPARAMS,0),DPARAMS[2],DSPGetInt16(DPARAMS,3));
    DOCUMENTATION
        Reads record Parameters:0,1 into memory page Parameter:2 ($00 6502 RAM, $80/$81 video RAM, $90 graphics RAM ;
        pages of Function 12,2) at address Parameters:3,4, the whole record (status byte included, check that it is 1).
        No copy through 6502 RAM. Errors : 1 no database or the range leaves the page, 2 storage error,
        3 record number out of range.

    FUNCTION 6 Update Record
        *DERROR = DBUpdate(DSPGetInt16(DPARAMS,0),DSPGetInt16(DPARAMS,2));
    DOCUMENTATION
        Overwrites record Parameters:0,1 with the record size bytes at Parameters:2,3 (6502 RAM below $FF00). Set byte 0
        to 1 to keep the record active (or to revive a deleted one). Errors : 1, 2, 3 as 40,5.

    FUNCTION 7 Append Record
        {
            uint16_t rec = 0;
            *DERROR = DBAppend(DSPGetInt16(DPARAMS,0),&rec);
            DSPSetInt16(DPARAMS,0,rec);
        }
    DOCUMENTATION
        Adds at the end the record size bytes at Parameters:0,1 (6502 RAM below $FF00) and returns its number in
        Parameters:0,1. Errors : 1 no database, bad address or 65535 records already, 2 storage error.

    FUNCTION 8 Delete Record
        *DERROR = DBDelete(DSPGetInt16(DPARAMS,0));
    DOCUMENTATION
        Marks record Parameters:0,1 deleted (status byte 0). The slot is not reused by 40,7 ; update it with 40,6 to
        reuse it. Errors : 1, 2, 3 as 40,5.

    FUNCTION 9 Search Record
        {
            uint16_t rec = 0;
            *DERROR = DBSearch(DSPGetInt16(DPARAMS,0),DPARAMS[2],DPARAMS[3],DSPGetInt16(DPARAMS,4),&rec);
            DSPSetInt16(DPARAMS,0,rec);
        }
    DOCUMENTATION
        Looks, from record Parameters:0,1 (0 is 1) onwards, for the first active record whose bytes
        Parameter:2 to Parameter:2 + Parameter:3 - 1 equal the Parameter:3 bytes at Parameters:4,5 (6502 RAM below
        $FF00). Returns its number in Parameters:0,1. The search is sequential, one storage read per record : it
        blocks the API until it ends ; to continue after a match call it again from the next record. The field must
        lie in the first 96 bytes of the record and inside it. Errors : 1 no database or bad field, 2 storage error,
        3 no match.
