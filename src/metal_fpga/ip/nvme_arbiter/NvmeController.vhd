library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.nvme_def.all;


entity NvmeController is
  port (
    pi_clk          : in  std_logic;
    pi_rst_n        : in  std_logic;

    p_cmdRd_TDATA   : in  std_logic_vector(167 downto 0);
    p_cmdRd_TVALID  : in  std_logic;
    p_cmdRd_TREADY  : out std_logic;

    p_rspRd_TDATA   : out std_logic_vector(7 downto 0);
    p_rspRd_TVALID  : out std_logic;
    p_rspRd_TREADY  : in  std_logic;

    p_cmdWr_TDATA   : in  std_logic_vector(167 downto 0);
    p_cmdWr_TVALID  : in  std_logic;
    p_cmdWr_TREADY  : out std_logic;

    p_rspWr_TDATA   : out std_logic_vector(7 downto 0);
    p_rspWr_TVALID  : out std_logic;
    p_rspWr_TREADY  : in  std_logic;

    p_nvme_ARID            : out std_logic_vector (0 downto 0);
    p_nvme_ARADDR          : out std_logic_vector (c_NvmeAxiAddrWidth-1 downto 0);
    p_nvme_ARLEN           : out std_logic_vector (c_AxiLenWidth-1 downto 0);
    p_nvme_ARSIZE          : out std_logic_vector (c_AxiSizeWidth-1 downto 0);
    p_nvme_ARBURST         : out std_logic_vector (c_AxiBurstWidth-1 downto 0);
    p_nvme_ARLOCK          : out std_logic_vector (c_AxiLockWidth-1 downto 0);
    p_nvme_ARCACHE         : out std_logic_vector (c_AxiCacheWidth-1 downto 0);
    p_nvme_ARPROT          : out std_logic_vector (c_AxiProtWidth-1 downto 0);
    p_nvme_ARQOS           : out std_logic_vector (c_AxiQosWidth-1 downto 0);
    p_nvme_ARREGION        : out std_logic_vector (c_AxiRegionWidth-1 downto 0);
    p_nvme_ARUSER          : out std_logic_vector (0 downto 0);
    p_nvme_ARVALID         : out std_logic;
    p_nvme_ARREADY         : in  std_logic;
    p_nvme_RID             : in  std_logic_vector (0 downto 0);
    p_nvme_RDATA           : in  std_logic_vector (c_NvmeAxiDataWidth-1 downto 0);
    p_nvme_RRESP           : in  std_logic_vector (c_AxiRespWidth-1 downto 0);
    p_nvme_RUSER           : in  std_logic_vector (0 downto 0);
    p_nvme_RLAST           : in  std_logic;
    p_nvme_RVALID          : in  std_logic;
    p_nvme_RREADY          : out std_logic;
    p_nvme_AWID            : out std_logic_vector (0 downto 0);
    p_nvme_AWADDR          : out std_logic_vector (c_NvmeAxiAddrWidth-1 downto 0);
    p_nvme_AWLEN           : out std_logic_vector (c_AxiLenWidth-1 downto 0);
    p_nvme_AWSIZE          : out std_logic_vector (c_AxiSizeWidth-1 downto 0);
    p_nvme_AWBURST         : out std_logic_vector (c_AxiBurstWidth-1 downto 0);
    p_nvme_AWLOCK          : out std_logic_vector (c_AxiLockWidth-1 downto 0);
    p_nvme_AWCACHE         : out std_logic_vector (c_AxiCacheWidth-1 downto 0);
    p_nvme_AWPROT          : out std_logic_vector (c_AxiProtWidth-1 downto 0);
    p_nvme_AWQOS           : out std_logic_vector (c_AxiQosWidth-1 downto 0);
    p_nvme_AWREGION        : out std_logic_vector (c_AxiRegionWidth-1 downto 0);
    p_nvme_AWUSER          : out std_logic_vector (0 downto 0);
    p_nvme_AWVALID         : out std_logic;
    p_nvme_AWREADY         : in  std_logic;
    p_nvme_WDATA           : out std_logic_vector (c_NvmeAxiDataWidth-1 downto 0);
    p_nvme_WSTRB           : out std_logic_vector (c_NvmeAxiStrbWidth-1 downto 0);
    p_nvme_WUSER           : out std_logic_vector (0 downto 0);
    p_nvme_WLAST           : out std_logic;
    p_nvme_WVALID          : out std_logic;
    p_nvme_WREADY          : in  std_logic;
    p_nvme_BID             : in  std_logic_vector (0 downto 0);
    p_nvme_BRESP           : in  std_logic_vector (c_AxiRespWidth-1 downto 0);
    p_nvme_BUSER           : in  std_logic_vector (0 downto 0);
    p_nvme_BVALID          : in  std_logic;
    p_nvme_BREADY          : out std_logic);
end NvmeController;

architecture NvmeController of NvmeController is

  -- Main State Machine
  type t_State is ( Idle, ReadStatus,
                    SendResRdPull, SendResRd, SendResWrPull, SendResWr,
                    WriteCommandPush, WriteCommand, Ready);
  signal s_state : t_State;

  -- Command Stream State Machine
  signal s_cmdRd : t_NvmeCmd;
  signal s_rspRd : t_NvmeRsp;
  signal s_cmdWr : t_NvmeCmd;
  signal s_rspWr : t_NvmeRsp;

  type t_CmdState is (IdleRd, IdleWr, AckRd, AckWr);
  signal s_cmdState : t_CmdState;

  signal s_cmdBufferAddr : t_NvmeBufferAddr;
  signal s_cmdDrive : t_NvmeDriveAddr;
  signal s_cmdBlockAddr : t_NvmeBlockAddr;
  signal s_cmdBlockCnt : t_NvmeBlockCount;
  signal s_cmdWrNotRd : std_logic;
  signal s_cmdValid : std_logic;
  signal s_cmdReady : std_logic;

  -- Drive Command Counter Block:
  signal s_driveCounters : t_NvmeDriveCounters;
  signal s_driveFree : t_NvmeDriveVector;
  signal s_driveInc : t_NvmeDriveVector;
  signal s_driveDec : t_NvmeDriveVector;
  signal s_driveCmdPending : std_logic;

  -- Cmd/Rsp Stream ID Queue:
  constant c_FIFOLogSize : integer := 9; -- 512 entries
  constant c_FIFOSize : integer := 2**c_FIFOLogSize;
  subtype t_FIFOPos is unsigned (c_FIFOLogSize-1 downto 0);
  constant c_FIFODataWidth : integer := c_NvmeDriveAddrWidth + 1; -- Drive Address and WrNotRd Bit
  subtype t_FIFOItem is unsigned(c_FIFODataWidth-1 downto 0);
  type t_FIFOBuf is array (c_FIFOSize-1 downto 0) of t_FIFOItem;
  signal s_fifoBuf : t_FIFOBuf;
  signal s_fifoPos : t_FIFOPos;
  signal s_fifoIn : t_FIFOItem;
  alias  a_fifoInDrive is s_fifoIn(3 downto 1);
  alias  a_fifoInWrNotRd is s_fifoIn(0);
  signal s_fifoPush : std_logic;
  signal s_fifoOut : t_FIFOItem;
  alias  a_fifoOutDrive is s_fifoOut(3 downto 1);
  alias  a_fifoOutWrNotRd is s_fifoOut(0);
  signal s_fifoPull : std_logic;

  -- Register Access Logic:
  type t_RegState is ( Idle,
                       RdStatus_AR, RdStatus_A, RdStatus_R,
                       WrBufLo_AW, WrBufLo_W, WrBufHi_AW, WrBufHi_W,
                       WrBlkLo_AW, WrBlkLo_W, WrBlkHi_AW, WrBlkHi_W,
                       WrCnt_AW, WrCnt_W, WrCmd_AW, WrCmd_W,
                       WrDone_A, Done);
  signal s_regState : t_RegState;
  signal s_regDoRdStatus : std_logic;
  signal s_regStatus : t_NvmeAxiData;
  signal s_regDoWrCommand : std_logic;
  signal s_regDone : std_logic;

begin

  -------------------------------------------------------------------------------
  -- Main State Machine
  -------------------------------------------------------------------------------
  process(pi_clk)
  begin
    if pi_clk'event and pi_clk = '1' then
      if pi_rst_n = '0' then
        s_state <= Idle;
      else
        case s_state is
          when Idle =>
            if s_cmdValid = '1' then
              s_state <= WriteCommandPush;
            elsif s_driveCmdPending = '1' then
              s_state <= ReadStatus;
            end if;

          when ReadStatus =>
            if s_regDone = '1' then
              if s_regStatus(0) = '1' and a_fifoOutWrNotRd = '0' then
                s_state <= SendResRdPull;
              elsif s_regStatus(0) = '1' and a_fifoOutWrNotRd = '1' then
                s_state <= SendResWrPull;
              else
                s_state <= Idle;
              end if;
            end if;
          when SendResRdPull =>
            if p_rspRd_TREADY = '1' then
              s_state <= ReadStatus;
            else
              s_state <= SendResRd;
            end if;
          when SendResRd =>
            if p_rspRd_TREADY = '1' then
              s_state <= ReadStatus;
            end if;
          when SendResWrPull =>
            if p_rspWr_TREADY = '1' then
              s_state <= ReadStatus;
            else
              s_state <= SendResWr;
            end if;
          when SendResWr =>
            if p_rspWr_TREADY = '1' then
              s_state <= ReadStatus;
            end if;

          when WriteCommandPush =>
            if s_regDone = '1' then
              s_state <= Ready;
            else
              s_state <= WriteCommand;
            end if;
          when WriteCommand =>
            if s_regDone = '1' then
              s_state <= Ready;
            end if;
          when Ready =>
            s_state <= ReadStatus;

        end case;
      end if;
    end if;
  end process;

  with s_state select s_regDoRdStatus <=
    '1' when ReadStatus,
    '0' when others;
  with s_state select s_regDoWrCommand <=
    '1' when WriteCommandPush,
    '1' when WriteCommand,
    '0' when others;
  with s_state select s_cmdReady <=
    '1' when Ready,
    '0' when others;

  a_fifoInWrNotRd <= s_cmdWrNotRd;
  a_fifoInDrive <= s_cmdDrive;
  with s_state select s_fifoPush <=
    '1' when WriteCommandPush,
    '0' when others;
  with s_state select s_fifoPull <=
    '1' when SendResRdPull,
    '1' when SendResWrPull,
    '0' when others;

  with s_state select s_driveDec <=
    f_driveDecode(a_fifoOutDrive) when SendResRdPull,
    f_driveDecode(a_fifoOutDrive) when SendResWrPull,
    (others => '0') when others;

  s_rspRd.status <= s_regStatus(1);
  with s_state select p_rspRd_TVALID <=
    '1' when SendResRdPull,
    '1' when SendResRd,
    '0' when others;
  s_rspWr.status <= s_regStatus(1);
  with s_state select p_rspWr_TVALID <=
    '1' when SendResWrPull,
    '1' when SendResWr,
    '0' when others;

  -----------------------------------------------------------------------------
  -- Command Stream State Machine
  -----------------------------------------------------------------------------
  process(pi_clk)
    variable v_cmdRdDrive : integer range 0 to c_NvmeDriveCount-1;
    variable v_cmdWrDrive : integer range 0 to c_NvmeDriveCount-1;
  begin
    v_cmdRdDrive := to_integer(s_cmdRd.drive);
    v_cmdWrDrive := to_integer(s_cmdWr.drive);
    if pi_clk'event and pi_clk = '1' then
      if pi_rst_n = '0' then
        p_cmdRd_TREADY <= '0';
        p_cmdWr_TREADY <= '0';
        s_driveInc <= (others => '0');
        s_cmdBufferAddr  <= (others => '0');
        s_cmdDrive       <= (others => '0');
        s_cmdBlockAddr   <= (others => '0');
        s_cmdBlockCnt    <= (others => '0');
        s_cmdWrNotRd     <= '0';
        s_cmdValid      <= '0';
        s_cmdState <= IdleRd;
      else
        s_driveInc <= (others => '0');
        p_cmdRd_TREADY <= '0';
        p_cmdWr_TREADY <= '0';
        case s_cmdState is

          when IdleRd =>
            if p_cmdRd_TVALID = '1' and v_cmdRdDrive < c_NvmeDrivesPresent and s_driveFree(v_cmdRdDrive) = '1' then
              p_cmdRd_TREADY           <= '1';
              s_driveInc(v_cmdRdDrive) <= '1';
              s_cmdBufferAddr          <= s_cmdRd.bufferAddr;
              s_cmdDrive               <= s_cmdRd.drive;
              s_cmdBlockAddr           <= s_cmdRd.blockAddr;
              s_cmdBlockCnt            <= s_cmdRd.blockCnt;
              s_cmdWrNotRd             <= '0';
              s_cmdValid               <= '1';
              s_cmdState               <= AckRd;
            elsif p_cmdWr_TVALID = '1' and v_cmdWrDrive < c_NvmeDrivesPresent and s_driveFree(v_cmdWrDrive) = '1' then
              p_cmdWr_TREADY           <= '1';
              s_driveInc(v_cmdWrDrive) <= '1';
              s_cmdBufferAddr          <= s_cmdWr.bufferAddr;
              s_cmdDrive               <= s_cmdWr.drive;
              s_cmdBlockAddr           <= s_cmdWr.blockAddr;
              s_cmdBlockCnt            <= s_cmdWr.blockCnt;
              s_cmdWrNotRd             <= '1';
              s_cmdValid               <= '1';
              s_cmdState               <= AckWr;
            end if;

          when IdleWr =>
            if p_cmdWr_TVALID = '1' and v_cmdWrDrive < c_NvmeDrivesPresent and s_driveFree(v_cmdWrDrive) = '1' then
              p_cmdWr_TREADY           <= '1';
              s_driveInc(v_cmdWrDrive) <= '1';
              s_cmdBufferAddr          <= s_cmdWr.bufferAddr;
              s_cmdDrive               <= s_cmdWr.drive;
              s_cmdBlockAddr           <= s_cmdWr.blockAddr;
              s_cmdBlockCnt            <= s_cmdWr.blockCnt;
              s_cmdWrNotRd             <= '1';
              s_cmdValid               <= '1';
              s_cmdState               <= AckWr;
            elsif p_cmdRd_TVALID = '1' and v_cmdRdDrive < c_NvmeDrivesPresent and s_driveFree(v_cmdRdDrive) = '1' then
              p_cmdRd_TREADY           <= '1';
              s_driveInc(v_cmdRdDrive) <= '1';
              s_cmdBufferAddr          <= s_cmdRd.bufferAddr;
              s_cmdDrive               <= s_cmdRd.drive;
              s_cmdBlockAddr           <= s_cmdRd.blockAddr;
              s_cmdBlockCnt            <= s_cmdRd.blockCnt;
              s_cmdWrNotRd             <= '0';
              s_cmdValid               <= '1';
              s_cmdState               <= AckRd;
            end if;

          when AckRd =>
            if s_cmdReady = '1' then
              s_cmdValid <= '0';
              s_cmdState <= IdleWr;
            end if;

          when AckWr =>
            if s_cmdReady = '1' then
              s_cmdValid <= '0';
              s_cmdState <= IdleRd;
            end if;

        end case;
      end if;
    end if;
  end process;

  -----------------------------------------------------------------------------
  -- Drive Command Counter Block
  -----------------------------------------------------------------------------
  process(pi_clk)
  begin
    if pi_clk'event and pi_clk = '1' then
      if pi_rst_n = '0' then
        s_driveCounters <= (others => c_NvmeCmdCounterZero);
        s_driveCmdPending <= '0';
      else
        s_driveCmdPending <= '0';
        for v_idx in 0 to c_NvmeDriveCount-1 loop
          if s_driveInc(v_idx) = '1' and s_driveDec(v_idx) = '0' then
            s_driveCounters(v_idx) <= s_driveCounters(v_idx) + c_NvmeCmdCounterOne;
          elsif s_driveInc(v_idx) = '0' and s_driveDec(v_idx) = '1' then
            s_driveCounters(v_idx) <= s_driveCounters(v_idx) - c_NvmeCmdCounterOne;
          end if;
          if s_driveCounters(v_idx) < c_NvmeCmdCounterLimit then
            s_driveFree(v_idx) <= '1';
          else
            s_driveFree(v_idx) <= '0';
          end if;
          if s_driveCounters(v_idx) > c_NvmeCmdCounterZero then
            s_driveCmdPending <= '1';
          end if;
        end loop;
      end if;
    end if;
  end process;

  -------------------------------------------------------------------------------
  -- Cmd/Rsp Stream ID Queue
  -------------------------------------------------------------------------------
  process(pi_clk)
  begin
    if pi_clk'event and pi_clk = '1' then
      if pi_rst_n = '0' then
        s_fifoBuf <= (others => (others => '0'));
        s_fifoPos <= (others => '0');
      else
        -- Assume that s_fifoPos never overflows, pending commands do not exceed 436 < 512
        if s_fifoPush = '1' and s_fifoPull = '1' then
          s_fifoBuf(s_fifoBuf'length-2 downto 0) <= s_fifoBuf(s_fifoBuf'length-1 downto 1); -- shift right
          s_fifoBuf(to_integer(s_fifoPos)) <= s_fifoIn;
        elsif s_fifoPush = '0' and s_fifoPull = '1' then
          s_fifoBuf(s_fifoBuf'length-2 downto 0) <= s_fifoBuf(s_fifoBuf'length-1 downto 1); -- shift right
          s_fifoPos <= s_fifoPos - to_unsigned(1, s_fifoPos'length);
        elsif s_fifoPush = '1' and s_fifoPull = '0' then
          s_fifoBuf(to_integer(s_fifoPos)) <= s_fifoIn;
          s_fifoPos <= s_fifoPos + to_unsigned(1, s_fifoPos'length);
        end if;
      end if;
    end if;
  end process;
  s_fifoOut <= s_fifoBuf(0);
  -- s_fifoValid <= f_logic(s_fifoPos > to_unsigned(0, s_fifoPos'length));

  -------------------------------------------------------------------------------
  -- Register Access Logic
  -------------------------------------------------------------------------------
  process(pi_clk)
  begin
    if pi_clk'event and pi_clk = '1' then
      if pi_rst_n = '0' then
        s_regState <= Idle;
      else
        case s_regState is

          when Idle =>
            if s_regDoRdStatus = '1' then
              s_regState <= RdStatus_AR;
            elsif s_regDoWrCommand = '1' then
              s_regState <= WrBufLo_AW;
            end if;

          when RdStatus_AR =>
            if p_nvme_ARREADY = '1' and p_nvme_RVALID = '1' then
              s_regStatus <= p_nvme_RDATA;
              s_regState <= Done;
            elsif p_nvme_ARREADY = '1' and p_nvme_RVALID = '0' then
              s_regState <= RdStatus_R;
            elsif p_nvme_ARREADY = '0' and p_nvme_RVALID = '1' then
              s_regStatus <= p_nvme_RDATA;
              s_regState <= RdStatus_A;
            end if;
          when RdStatus_A =>
            if p_nvme_ARREADY = '1' then
              s_regState <= Done;
            end if;
          when RdStatus_R =>
            if p_nvme_RVALID = '1' then
              s_regStatus <= p_nvme_RDATA;
              s_regState <= Done;
            end if;

          when WrBufLo_AW =>
            if p_nvme_AWREADY = '1' and p_nvme_WREADY = '1' then
              s_regState <= WrBufHi_W;
            elsif p_nvme_AWREADY = '1' and p_nvme_WREADY = '0' then
              s_regState <= WrBufLo_W;
            elsif p_nvme_AWREADY = '0' and p_nvme_WREADY = '1' then
              s_regState <= WrBufHi_AW;
            end if;
          when WrBufLo_W =>
            if p_nvme_WREADY = '1' then
              s_regState <= WrBufHi_W;
            end if;
          when WrBufHi_AW =>
            if p_nvme_AWREADY = '1' and p_nvme_WREADY = '1' then
              s_regState <= WrBlkLo_W;
            elsif p_nvme_AWREADY = '1' and p_nvme_WREADY = '0' then
              s_regState <= WrBufHi_W;
            elsif p_nvme_AWREADY = '0' and p_nvme_WREADY = '1' then
              s_regState <= WrBlkLo_AW;
            end if;
          when WrBufHi_W =>
            if p_nvme_WREADY = '1' then
              s_regState <= WrBlkLo_W;
            end if;

          when WrBlkLo_AW =>
            if p_nvme_AWREADY = '1' and p_nvme_WREADY = '1' then
              s_regState <= WrBlkHi_W;
            elsif p_nvme_AWREADY = '1' and p_nvme_WREADY = '0' then
              s_regState <= WrBlkLo_W;
            elsif p_nvme_AWREADY = '0' and p_nvme_WREADY = '1' then
              s_regState <= WrBlkHi_AW;
            end if;
          when WrBlkLo_W =>
            if p_nvme_WREADY = '1' then
              s_regState <= WrBlkHi_W;
            end if;
          when WrBlkHi_AW =>
            if p_nvme_AWREADY = '1' and p_nvme_WREADY = '1' then
              s_regState <= WrCnt_W;
            elsif p_nvme_AWREADY = '1' and p_nvme_WREADY = '0' then
              s_regState <= WrBlkHi_W;
            elsif p_nvme_AWREADY = '0' and p_nvme_WREADY = '1' then
              s_regState <= WrCnt_AW;
            end if;
          when WrBlkHi_W =>
            if p_nvme_WREADY = '1' then
              s_regState <= WrCnt_W;
            end if;

          when WrCnt_AW =>
            if p_nvme_AWREADY = '1' and p_nvme_WREADY = '1' then
              s_regState <= WrCmd_W;
            elsif p_nvme_AWREADY = '1' and p_nvme_WREADY = '0' then
              s_regState <= WrCnt_W;
            elsif p_nvme_AWREADY = '0' and p_nvme_WREADY = '1' then
              s_regState <= WrCmd_AW;
            end if;
          when WrCnt_W =>
            if p_nvme_WREADY = '1' then
              s_regState <= WrCmd_W;
            end if;
          when WrCmd_AW =>
            if p_nvme_AWREADY = '1' and p_nvme_WREADY = '1' then
              s_regState <= Done;
            elsif p_nvme_AWREADY = '1' and p_nvme_WREADY = '0' then
              s_regState <= WrCmd_W;
            elsif p_nvme_AWREADY = '0' and p_nvme_WREADY = '1' then
              s_regState <= WrDone_A;
            end if;
          when WrCmd_W =>
            if p_nvme_WREADY = '1' then
              s_regState <= Done;
            end if;

          when WrDone_A =>
            if p_nvme_AWREADY = '1' then
              s_regState <= Done;
            end if;

          when Done =>
            s_regState <= Idle;

        end case;
      end if;
    end if;
  end process;

  p_nvme_ARADDR <= x"00000004";
  p_nvme_ARLEN <= x"00";
  with s_regState select p_nvme_ARVALID <=
    '1' when RdStatus_AR,
    '1' when RdStatus_A,
    '0' when others;
  with s_regState select p_nvme_RREADY <=
    '1' when RdStatus_AR,
    '1' when RdStatus_R,
    '0' when others;

  p_nvme_AWADDR <= x"00000000";
  p_nvme_AWLEN <= x"05";
  with s_regState select p_nvme_AWVALID <=
    '1' when WrBufLo_AW,
    '1' when WrBufHi_AW,
    '1' when WrBlkLo_AW,
    '1' when WrBlkHi_AW,
    '1' when WrCnt_AW,
    '1' when WrCmd_AW,
    '1' when WrDone_A,
    '0' when others;
  with s_regState select p_nvme_WDATA <=
    std_logic_vector(s_cmdBufferAddr(31 downto  0)) when WrBufLo_AW,
    std_logic_vector(s_cmdBufferAddr(31 downto  0)) when WrBufLo_W,
    std_logic_vector(s_cmdBufferAddr(63 downto 31)) when WrBufHi_AW,
    std_logic_vector(s_cmdBufferAddr(63 downto 31)) when WrBufHi_W,
    std_logic_vector(s_cmdBlockAddr(31 downto  0)) when WrBlkLo_AW,
    std_logic_vector(s_cmdBlockAddr(31 downto  0)) when WrBlkLo_W,
    std_logic_vector(s_cmdBlockAddr(63 downto 31)) when WrBlkHi_AW,
    std_logic_vector(s_cmdBlockAddr(63 downto 31)) when WrBlkHi_W,
    std_logic_vector(s_cmdBlockCnt(31 downto 0)) when WrCnt_AW,
    std_logic_vector(s_cmdBlockCnt(31 downto 0)) when WrCnt_W,
    f_nvmeControlWord(s_cmdWrNotRd, s_cmdDrive) when WrCmd_AW,
    f_nvmeControlWord(s_cmdWrNotRd, s_cmdDrive) when WrCmd_W,
    (others => '0') when others;
  with s_regState select p_nvme_WLAST <=
    '1' when WrCmd_AW,
    '1' when WrCmd_W,
    '0' when others;
  with s_regState select p_nvme_WVALID <=
    '1' when WrBufLo_AW,
    '1' when WrBufLo_W,
    '1' when WrBufHi_AW,
    '1' when WrBufHi_W,
    '1' when WrBlkLo_AW,
    '1' when WrBlkLo_W,
    '1' when WrBlkHi_AW,
    '1' when WrBlkHi_W,
    '1' when WrCnt_AW,
    '1' when WrCnt_W,
    '1' when WrCmd_AW,
    '1' when WrCmd_W,
    '0' when others;

  -------------------------------------------------------------------------------
  -- Wrapper Code
  -------------------------------------------------------------------------------
  s_cmdRd <= f_unpackNvmeCmd(p_cmdRd_TDATA);
  -- p_cmdRd_TVALID to Command State Machine
  -- p_cmdRd_TREADY from Command State Machine
  p_rspRd_TDATA <= f_packNvmeRsp(s_rspRd);
  -- p_rspRd_TVALID from Main State Machine
  -- p_rspRd_TREADY to Main State Machine
  s_cmdWr <= f_unpackNvmeCmd(p_cmdWr_TDATA);
  -- p_cmdWr_TVALID to Comman State Machine
  -- p_cmdWr_TREADY from Command State Machine
  p_rspWr_TDATA <= f_packNvmeRsp(s_rspWr);
  -- p_rspWr_TVALID from Main State Machine
  -- p_rspWr_TREADY to Main State Machine

  p_nvme_ARID     <= (others => '0');
  -- p_nvme_ARADDR from Register State Machine
  -- p_nvme_ARLEN from Register State Machine
  p_nvme_ARSIZE   <= c_NvmeAxiFullSize;
  p_nvme_ARBURST  <= c_AxiBurstIncr;
  p_nvme_ARLOCK   <= c_AxiNullLock;
  p_nvme_ARCACHE  <= c_AxiNullCache;
  p_nvme_ARPROT   <= c_AxiNullProt;
  p_nvme_ARQOS    <= c_AxiNullQos;
  p_nvme_ARREGION <= c_AxiNullRegion;
  p_nvme_ARUSER   <= (others => '0');
  -- p_nvme_ARVALID from Register State Machine
  -- p_nvme_ARREADY to Register State Machine
  -- (p_nvme_RID)
  -- p_nvme_RDATA to Register State Machine
  -- (p_nvme_RRESP)
  -- (p_nvme_RUSER)
  -- (p_nvme_RLAST)
  -- p_nvme_RVALID to Register State Machine
  -- p_nvme_RREADY from Register State Machine
  p_nvme_AWID     <= (others => '0');
  -- p_nvme_AWADDR from Register State Machine
  -- p_nvme_AWLEN from Register State Machine
  p_nvme_AWSIZE   <= c_NvmeAxiFullSize;
  p_nvme_AWBURST  <= c_AxiBurstIncr;
  p_nvme_AWLOCK   <= c_AxiNullLock;
  p_nvme_AWCACHE  <= c_AxiNullCache;
  p_nvme_AWPROT   <= c_AxiNullProt;
  p_nvme_AWQOS    <= c_AxiNullQos;
  p_nvme_AWREGION <= c_AxiNullRegion;
  p_nvme_AWUSER   <= (others => '0');
  -- p_nvme_AWVALID from Register State Machine
  -- p_nvme_AWREADY to Register State Machine
  -- p_nvme_WDATA from Register State Machine
  p_nvme_WSTRB    <= (others => '1');
  p_nvme_WUSER    <= (others => '0');
  -- p_nvme_WLAST from Register State Machine
  -- p_nvme_WVALID from Register State Machine
  -- p_nvme_WREADY to Register State Machine
  -- (p_nvme_BID)
  -- (p_nvme_BRESP)
  -- (p_nvme_BUSER)
  -- (p_nvme_BVALID)
  p_nvme_BREADY   <= '1';

end NvmeController;
