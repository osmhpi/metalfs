library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

package nvme_def is

  function f_clog2 (v_value : natural) return positive;

  -----------------------------------------------------------------------------
  -- General Axi Declarations
  -----------------------------------------------------------------------------

  -- byte address bits of the boundary a burst may not cross(4KB blocks)
  constant c_AxiBurstAlignWidth : integer := 12;

  constant c_AxiLenWidth    : integer := 8;
  subtype t_AxiLen is std_logic_vector(c_AxiLenWidth-1 downto 0);
  constant c_AxiNullLen : t_AxiLen := (others => '0');

  constant c_AxiSizeWidth   : integer := 3;
  subtype t_AxiSize is std_logic_vector(c_AxiSizeWidth-1 downto 0);

  constant c_AxiBurstWidth  : integer := 2;
  subtype t_AxiBurst is std_logic_vector(c_AxiBurstWidth-1 downto 0);
  constant c_AxiBurstFixed : t_AxiBurst := "00";
  constant c_AxiBurstIncr : t_AxiBurst := "01";
  constant c_AxiBurstWrap : t_AxiBurst := "10";
  constant c_AxiNullBurst : t_AxiBurst := c_AxiBurstFixed;

  constant c_AxiLockWidth  : integer := 2;
  subtype t_AxiLock is std_logic_vector(c_AxiLockWidth-1 downto 0);
  constant c_AxiLockNormal : t_AxiLock := "00";
  constant c_AxiLockExclusive : t_AxiLock := "01";
  constant c_AxiLockLocked : t_AxiLock := "10";
  constant c_AxiNullLock   : t_AxiLock := c_AxiLockNormal;

  constant c_AxiCacheWidth  : integer := 4;
  subtype t_AxiCache is std_logic_vector(c_AxiCacheWidth-1 downto 0);
  constant c_AxiNullCache  : t_AxiCache  := "0010"; -- Normal, NoCache, NoBuffer

  constant c_AxiProtWidth   : integer := 3;
  subtype t_AxiProt is std_logic_vector(c_AxiProtWidth-1 downto 0);
  constant c_AxiNullProt   : t_AxiProt   := "000";  -- Unprivileged, Non-Sec, Data

  constant c_AxiQosWidth    : integer := 4;
  subtype t_AxiQos is std_logic_vector(c_AxiQosWidth-1 downto 0);
  constant c_AxiNullQos    : t_AxiQos    := "0000"; -- No QOS

  constant c_AxiRegionWidth : integer := 4;
  subtype t_AxiRegion is std_logic_vector(c_AxiRegionWidth-1 downto 0);
  constant c_AxiNullRegion : t_AxiRegion := "0000"; -- Default Region

  constant c_AxiRespWidth : integer := 2;
  subtype t_AxiResp is std_logic_vector(c_AxiRespWidth-1 downto 0);
  constant c_AxiRespOkay   : t_AxiResp := "00";
  constant c_AxiRespExOkay : t_AxiResp := "01";
  constant c_AxiRespSlvErr : t_AxiResp := "10";
  constant c_AxiRespDecErr : t_AxiResp := "11";

  -------------------------------------------------------------------------------
  -- Axi Interface: Nvme
  -------------------------------------------------------------------------------

  constant c_NvmeAxiDataWidth : integer := 32;
  subtype t_NvmeAxiData is std_logic_vector (c_NvmeAxiDataWidth-1 downto 0);

  constant c_NvmeAxiStrbWidth : integer := c_NvmeAxiDataWidth/8;
  subtype t_NvmeAxiStrb is std_logic_vector (c_NvmeAxiStrbWidth-1 downto 0);

  constant c_NvmeAxiAddrWidth : integer := 32;
  subtype t_NvmeAxiAddr is std_logic_vector (c_NvmeAxiAddrWidth-1 downto 0);

  constant c_NvmeAxiByteAddrWidth : integer := f_clog2(c_NvmeAxiStrbWidth);
  constant c_NvmeAxiFullSize : t_AxiSize := std_logic_vector(to_unsigned(c_NvmeAxiByteAddrWidth, t_AxiSize'length));
  subtype t_NvmeAxiByteAddr is std_logic_vector (c_NvmeAxiByteAddrWidth-1 downto 0);

  constant c_NvmeAxiRespWidth : integer := 2;
  subtype t_NvmeAxiResp is std_logic_vector (c_NvmeAxiRespWidth-1 downto 0);

  -------------------------------------------------------------------------------
  -- Internal Definitions
  -------------------------------------------------------------------------------
  constant c_NvmeBufferAddrWidth : integer := 64;
  subtype t_NvmeBufferAddr is unsigned (c_NvmeBufferAddrWidth-1 downto 0);

  constant c_NvmeBlockAddrWidth : integer := 64;
  subtype t_NvmeBlockAddr  is unsigned (c_NvmeBlockAddrWidth-1 downto 0);

  constant c_NvmeBlockCountWidth : integer := 32;
  subtype t_NvmeBlockCount is unsigned (c_NvmeBlockCountWidth-1 downto 0);

  constant c_NvmeDriveAddrWidth : integer := 3;
  subtype t_NvmeDriveAddr  is unsigned (c_NvmeDriveAddrWidth-1 downto 0);
  constant c_NvmeDriveCount : integer := 2**c_NvmeDriveAddrWidth;

  constant c_NvmeDrivesPresent : integer := 2;

  subtype t_NvmeCmdCounter is unsigned (8 downto 0);
  constant c_NvmeCmdCounterZero : t_NvmeCmdCounter := to_unsigned(0, t_NvmeCmdCounter'length);
  constant c_NvmeCmdCounterOne : t_NvmeCmdCounter := to_unsigned(1, t_NvmeCmdCounter'length);
  constant c_NvmeCmdCounterLimit : t_NvmeCmdCounter := to_unsigned(218, t_NvmeCmdCounter'length);
  type t_NvmeDriveCounters is array (c_NvmeDriveCount-1 downto 0) of t_NvmeCmdCounter;
  subtype t_NvmeDriveVector is unsigned (c_NvmeDriveCount-1 downto 0);
  function f_driveDecode(v_pos : t_NvmeDriveAddr) return t_NvmeDriveVector;

  function f_nvmeControlWord(writeNotRead : std_logic; driveAddr : t_NvmeDriveAddr) return t_NvmeAxiData;

  type t_NvmeCmd is record
    bufferAddr : t_NvmeBufferAddr;
    drive : t_NvmeDriveAddr;
    blockAddr : t_NvmeBlockAddr;
    blockCnt : t_NvmeBlockCount;
  end record;
  constant c_NvmeCmdPayloadRawWidth : integer := t_NvmeBufferAddr'length + t_NvmeDriveAddr'length + t_NvmeBlockAddr'length + t_NvmeBlockCount'length;
  constant c_NvmeCmdPayloadWidth : integer := ((c_NvmeCmdPayloadRawWidth - 1) / 8 + 1) * 8; -- round to next higher byte multiple
  subtype t_NvmeCmdPayload is std_logic_vector(c_NvmeCmdPayloadWidth-1 downto 0);
  function f_unpackNvmeCmd(payload : t_NvmeCmdPayload) return t_NvmeCmd;

  type t_NvmeRsp is record
    status : std_logic; -- 0: success, 1: error
  end record;
  constant c_NvmeRspPayloadRawWidth : integer := 1;
  constant c_NvmeRspPayloadWidth : integer := ((c_NvmeRspPayloadRawWidth - 1) / 8 + 1) * 8; -- round to next higher byte multiple
  subtype t_NvmeRspPayload is std_logic_vector (c_NvmeRspPayloadWidth-1 downto 0);
  function f_packNvmeRsp(rsp : t_NvmeRsp) return t_NvmeRspPayload;

end nvme_def;

package body nvme_def is

  function f_clog2 (v_value : natural) return positive is
    variable v_count  : positive;
  begin
    v_count := 1;
    while v_value > 2**v_count loop
      v_count := v_count + 1;
    end loop;
    return v_count;
  end f_clog2;

  function f_nvmeControlWord(writeNotRead : std_logic; driveAddr : t_NvmeDriveAddr) return t_NvmeAxiData is
    variable v_return : t_NvmeAxiData;
  begin
    v_return := (others => '0');
    v_return(0) := writeNotRead;
    v_return(7 downto 5) := std_logic_vector(driveAddr);
    v_return(4) := '1'; -- indicate IO instead of Admin Queue
    return v_return;
  end f_nvmeControlWord;

  function f_driveDecode(v_pos : t_NvmeDriveAddr) return t_NvmeDriveVector is
    variable v_return : t_NvmeDriveVector;
  begin
    v_return := (others => '0');
    v_return(to_integer(v_pos)) := '1';
    return v_return;
  end f_driveDecode;

  function f_unpackNvmeCmd(payload : t_NvmeCmdPayload) return t_NvmeCmd is
    constant c_BufBegin : integer := 0;
    constant c_BlkBegin : integer := c_BufBegin + t_NvmeBufferAddr'length;
    constant c_CntBegin : integer := c_BlkBegin + t_NvmeBlockAddr'length;
    constant c_DrvBegin : integer := c_CntBegin + t_NvmeBlockCount'length;
    constant c_End      : integer := c_DrvBegin + t_NvmeDriveAddr'length;
    variable v_return   : t_NvmeCmd;
  begin
    v_return.bufferAddr := unsigned(payload(c_BlkBegin-1 downto c_BufBegin));
    v_return.blockAddr  := unsigned(payload(c_CntBegin-1 downto c_BlkBegin));
    v_return.blockCnt   := unsigned(payload(c_DrvBegin-1 downto c_CntBegin));
    v_return.drive      := unsigned(payload(c_End-1      downto c_DrvBegin));
    return v_return;
  end f_unpackNvmeCmd;

  function f_packNvmeRsp(rsp : t_NvmeRsp) return t_NvmeRspPayload is
    variable v_return : t_NvmeRspPayload;
  begin
    v_return := (others => '0');
    v_return(0) := rsp.status;
    return v_return;
  end f_packNvmeRsp;

end nvme_def;
