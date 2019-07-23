library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;


use work.fosi_ctrl.all;
use work.fosi_nvme.all;


entity NvmeController is
  port (
    pi_clk      : in  std_logic;
    pi_rst_n    : in  std_logic;

    pi_cmdRd_ms : in  t_NvmeCommandStream_ms;
    po_cmdRd_sm : out t_NvmeCommandStream_sm;
    po_rspRd_ms : out t_NvmeResponseStream_ms;
    pi_rspRd_sm : in  t_NvmeResponseStream_sm;

    pi_cmdWr_ms : in  t_NvmeCommandStream_ms;
    po_cmdWr_sm : out t_NvmeCommandStream_sm;
    po_rspWr_ms : out t_NvmeResponseStream_ms;
    pi_rspWr_sm : in  t_NvmeResponseStream_sm;

    po_nvme_ms  : out t_Ctrl_ms;
    pi_nvme_sm  : in  t_Ctrl_sm);
end NvmeController;

architecture NvmeController of NvmeController is

  function f_decode(v_pos : t_NvmeDriveAddr) return t_NvmeDriveVector is
    variable v_return : t_NvmeDriveVector;
  begin
    v_return := (others => '0');
    v_return(to_integer(v_pos)) := '1';
    return v_return;
  end f_decode;

  -- Main State Machine
  type t_State is ( Idle, ReadStatus,
                    SendResRdPull, SendResRd, SendResWrPull, SendResWr,
                    WriteBufferAddrLoPush, WriteBufferAddrLo, WriteBufferAddrHi,
                    WriteBlockAddrLo, WriteBlockAddrHi,
                    WriteBlockCnt, WriteControl, NextReady);
  signal s_state : t_State;

  -- Command Stream State Machine

  type t_CmdState is (IdleRd, IdleWr, AckRd, AckWr);
  signal s_cmdState : t_CmdState;

  signal s_cmdBufferAddr : t_NvmeBufferAddr;
  signal s_cmdDrive : t_NvmeDriveAddr;
  signal s_cmdBlockAddr : t_NvmeBlockAddr;
  signal s_cmdBlockCnt : t_NvmeBlockCount;
  signal s_cmdWrNotRd : std_logic;
  signal s_cmdValid : std_logic;
  signal s_cmdReady : std_logic;
  signal s_cmdControl : t_CtrlData;

  signal s_driveCounters : t_NvmeDriveCounters;
  signal s_driveFree : t_NvmeDriveVector;
  signal s_driveInc : t_NvmeDriveVector;
  signal s_driveDec : t_NvmeDriveVector;

  -- Cmd/Rsp Stream ID Queue
  constant c_FIFOLogSize : integer := 9; -- 512 entries
  constant c_FIFOSize : integer := 2**c_FIFOLogSize;
  subtype t_FIFOPos is unsigned (c_FIFOLogSize-1 downto 0);
  constant c_FIFODataWidth : integer := c_NvmeDriveAddrWidth + 1; -- Drive Address and WrNotRd Bit
  subtype t_FIFOItem is unsigned(c_FIFODataWidth-1 downto 0);
  type t_FIFOBuf is array (c_FIFOSize-1 downto 0) of t_FIFOItem;
  signal s_fifoBuf : t_FIFOBuf;
  signal s_fifoPos : t_FIFOPos;
  signal s_fifoIn : t_FIFOItem;
  signal s_fifoPush : std_logic;
  signal s_fifoOut : t_FIFOItem;
  signal s_fifoPull : std_logic;

  -- Register Access Logic
  type t_RegState is (Idle, ReadWaitAWaitR, ReadWaitA, ReadWaitR, WriteWaitAWaitW, WriteWaitA, WriteWaitW, Ready);
  signal s_regState : t_RegState;
  signal s_regAddr : t_CtrlAddr;
  signal s_regWrData : t_CtrlData;
  signal s_regWrNotRd : std_logic;
  signal s_regValid : std_logic;
  signal s_regRdData : t_CtrlData; -- valid from regReady assertion up to next regValid
  signal s_regReady : std_logic;

begin

  -------------------------------------------------------------------------------
  -- Main State Machine (Moore Type)
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
              s_state <= WriteBufferAddrLoPush;
            else
              s_state <= ReadStatus;
            end if;

          when ReadStatus =>
            if s_regReady = '1' then
              if s_regRdData(0) = '1' and s_fifoOut(0) = '0' then
                s_state <= SendResRdPull;
              elsif s_regRdData(0) = '1' and s_fifoOut(0) = '1' then
                s_state <= SendResWrPull;
              else
                s_state <= Idle;
              end if;
            end if;
          when SendResRdPull =>
            if pi_rspRd_sm.ready = '1' then
              s_state <= Idle;
            else
              s_state <= SendResRd;
            end if;
          when SendResRd =>
            if pi_rspRd_sm.ready = '1' then
              s_state <= Idle;
            end if;
          when SendResWrPull =>
            if pi_rspWr_sm.ready = '1' then
              s_state <= Idle;
            else
              s_state <= SendResWr;
            end if;
          when SendResWr =>
            if pi_rspWr_sm.ready = '1' then
              s_state <= Idle;
            end if;

          when WriteBufferAddrLoPush =>
            if s_regReady = '1' then
              s_state <= WriteBufferAddrHi;
            else
              s_state <= WriteBufferAddrLo;
            end if;
          when WriteBufferAddrLo =>
            if s_regReady = '1' then
              s_state <= WriteBufferAddrHi;
            end if;
          when WriteBufferAddrHi =>
            if s_regReady = '1' then
              s_state <= WriteBlockAddrLo;
            end if;
          when WriteBlockAddrLo =>
            if s_regReady = '1' then
              s_state <= WriteBlockAddrHi;
            end if;
          when WriteBlockAddrHi =>
            if s_regReady = '1' then
              s_state <= WriteBlockCnt;
            end if;
          when WriteBlockCnt =>
            if s_regReady = '1' then
              s_state <= WriteControl;
            end if;
          when WriteControl =>
            if s_regReady = '1' then
              s_state <= NextReady;
            end if;
          when NextReady =>
            s_state <= ReadStatus;

        end case;
      end if;
    end if;
  end process;

  with s_state select s_cmdReady <=
    '1' when NextReady,
    '0' when others;
  with s_state select s_regAddr <=
    to_unsigned(0, s_regAddr'length) when WriteBufferAddrLoPush,
    to_unsigned(0, s_regAddr'length) when WriteBufferAddrLo,
    to_unsigned(4, s_regAddr'length) when WriteBufferAddrHi,
    to_unsigned(8, s_regAddr'length) when WriteBlockAddrLo,
    to_unsigned(12, s_regAddr'length) when WriteBlockAddrHi,
    to_unsigned(16, s_regAddr'length) when WriteBlockCnt,
    to_unsigned(20, s_regAddr'length) when WriteControl,
    to_unsigned(4, s_regAddr'length) when ReadStatus,
    (others => '0') when others;
  with s_state select s_regWrData <=
    s_cmdBufferAddr(31 downto  0) when WriteBufferAddrLoPush,
    s_cmdBufferAddr(31 downto  0) when WriteBufferAddrLo,
    s_cmdBufferAddr(63 downto 32) when WriteBufferAddrHi,
    s_cmdBlockAddr(31 downto  0) when WriteBlockAddrLo,
    s_cmdBlockAddr(63 downto 32) when WriteBlockAddrHi,
    s_cmdBlockCnt(31 downto 0) when WriteBlockCnt,
    s_cmdControl(31 downto 0) when WriteControl,
    (others => '0') when others;
  with s_state select s_regWrNotRd <=
    '1' when WriteBufferAddrLoPush,
    '1' when WriteBufferAddrLo,
    '1' when WriteBufferAddrHi,
    '1' when WriteBlockAddrLo,
    '1' when WriteBlockAddrHi,
    '1' when WriteBlockCnt,
    '1' when WriteControl,
    '0' when others;
  with s_state select s_regValid <=
    '1' when WriteBufferAddrLoPush,
    '1' when WriteBufferAddrLo,
    '1' when WriteBufferAddrHi,
    '1' when WriteBlockAddrLo,
    '1' when WriteBlockAddrHi,
    '1' when WriteBlockCnt,
    '1' when WriteControl,
    '1' when ReadStatus,
    '0' when others;

  s_fifoIn <= s_cmdDrive & s_cmdWrNotRd;
  with s_state select s_fifoPush <=
    '1' when WriteBufferAddrLoPush,
    '0' when others;
  with s_state select s_fifoPull <=
    '1' when SendResRdPull,
    '1' when SendResWrPull,
    '0' when others;

  with s_state select s_driveDec <=
    f_decode(s_fifoOut(3 downto 1)) when SendResRdPull,
    f_decode(s_fifoOut(3 downto 1)) when SendResWrPull,
    (others => '0') when others;

  po_rspRd_ms.status <= s_regRdData(1);
  with s_state select po_rspRd_ms.valid <=
    '1' when SendResRdPull,
    '1' when SendResRd,
    '0' when others;
  po_rspWr_ms.status <= s_regRdData(1);
  with s_state select po_rspWr_ms.valid <=
    '1' when SendResWrPull,
    '1' when SendResWr,
    '0' when others;

  -----------------------------------------------------------------------------
  -- Command Stream State Machine
  -----------------------------------------------------------------------------
  s_cmdControl <= x"000000" & s_cmdDrive & "1000" & s_cmdWrNotRd;
  process(pi_clk)
    variable v_cmdRdDrive : integer range 0 to c_NvmeDriveCount-1;
    variable v_cmdWrDrive : integer range 0 to c_NvmeDriveCount-1;
  begin
    v_cmdRdDrive := to_integer(pi_cmdRd_ms.drive);
    v_cmdWrDrive := to_integer(pi_cmdWr_ms.drive);
    if pi_clk'event and pi_clk = '1' then
      if pi_rst_n = '0' then
        po_cmdRd_sm.ready <= '0';
        po_cmdWr_sm.ready <= '0';
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
        po_cmdRd_sm.ready <= '0';
        po_cmdWr_sm.ready <= '0';
        case s_cmdState is

          when IdleRd =>
            if pi_cmdRd_ms.valid = '1' and v_cmdRdDrive < c_NvmeDrivesPresent and s_driveFree(v_cmdRdDrive) = '1' then
              po_cmdRd_sm.ready <= '1';
              s_driveInc(v_cmdRdDrive) <= '1';
              s_cmdBufferAddr  <= pi_cmdRd_ms.bufferAddr;
              s_cmdDrive       <= pi_cmdRd_ms.drive;
              s_cmdBlockAddr   <= pi_cmdRd_ms.blockAddr;
              s_cmdBlockCnt    <= pi_cmdRd_ms.blockCnt;
              s_cmdWrNotRd     <= '0';
              s_cmdValid      <= '1';
              s_cmdState <= AckRd;
            elsif pi_cmdWr_ms.valid = '1' and v_cmdWrDrive < c_NvmeDrivesPresent and s_driveFree(v_cmdWrDrive) = '1' then
              po_cmdWr_sm.ready <= '1';
              s_driveInc(v_cmdWrDrive) <= '1';
              s_cmdBufferAddr  <= pi_cmdWr_ms.bufferAddr;
              s_cmdDrive       <= pi_cmdWr_ms.drive;
              s_cmdBlockAddr   <= pi_cmdWr_ms.blockAddr;
              s_cmdBlockCnt    <= pi_cmdWr_ms.blockCnt;
              s_cmdWrNotRd     <= '1';
              s_cmdValid      <= '1';
              s_cmdState <= AckWr;
            end if;

          when IdleWr =>
            if pi_cmdWr_ms.valid = '1' and v_cmdWrDrive < c_NvmeDrivesPresent and s_driveFree(v_cmdWrDrive) = '1' then
              po_cmdWr_sm.ready <= '1';
              s_driveInc(v_cmdWrDrive) <= '1';
              s_cmdBufferAddr <= pi_cmdWr_ms.bufferAddr;
              s_cmdDrive      <= pi_cmdWr_ms.drive;
              s_cmdBlockAddr  <= pi_cmdWr_ms.blockAddr;
              s_cmdBlockCnt   <= pi_cmdWr_ms.blockCnt;
              s_cmdWrNotRd    <= '1';
              s_cmdValid      <= '1';
              s_cmdState <= AckWr;
            elsif pi_cmdRd_ms.valid = '1' and v_cmdRdDrive < c_NvmeDrivesPresent and s_driveFree(v_cmdRdDrive) = '1' then
              po_cmdRd_sm.ready <= '1';
              s_driveInc(v_cmdRdDrive) <= '1';
              s_cmdBufferAddr <= pi_cmdRd_ms.bufferAddr;
              s_cmdDrive      <= pi_cmdRd_ms.drive;
              s_cmdBlockAddr  <= pi_cmdRd_ms.blockAddr;
              s_cmdBlockCnt   <= pi_cmdRd_ms.blockCnt;
              s_cmdWrNotRd    <= '0';
              s_cmdValid      <= '1';
              s_cmdState <= AckRd;
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
      else
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
  po_nvme_ms.araddr <= s_regAddr;
  po_nvme_ms.awaddr <= s_regAddr;
  po_nvme_ms.wdata <= s_regWrData;
  po_nvme_ms.wstrb <= (others => '1');
  po_nvme_ms.bready <= '1';
  process(pi_clk)
  begin
    if pi_clk'event and pi_clk = '1' then
      if pi_rst_n = '0' then
        s_regState <= Idle;
        po_nvme_ms.arvalid <= '0';
        po_nvme_ms.rready <= '0';
        po_nvme_ms.awvalid <= '0';
        po_nvme_ms.wvalid <= '0';
        s_regReady <= '0';
      else
        case s_regState is
          when Idle =>
            if s_regValid = '1' and s_regWrNotRd = '0' then
              po_nvme_ms.arvalid <= '1';
              po_nvme_ms.rready <= '1';
              s_regState <= ReadWaitAWaitR;
            elsif s_regValid = '1' and s_regWrNotRd = '1' then
              po_nvme_ms.awvalid <= '1';
              po_nvme_ms.wvalid <= '1';
              s_regState <= WriteWaitAWaitW;
            end if;

          when ReadWaitAWaitR =>
            if pi_nvme_sm.arready = '1' and pi_nvme_sm.rvalid = '1' then
              po_nvme_ms.arvalid <= '0';
              po_nvme_ms.rready <= '0';
              s_regRdData <= pi_nvme_sm.rdata;
              s_regReady <= '1';
              s_regState <= Ready;
            elsif pi_nvme_sm.arready = '0' and pi_nvme_sm.rvalid = '1' then
              po_nvme_ms.rready <= '0';
              s_regRdData <= pi_nvme_sm.rdata;
              s_regState <= ReadWaitA;
            elsif pi_nvme_sm.arready = '1' and pi_nvme_sm.rvalid = '0' then
              po_nvme_ms.arvalid <= '0';
              s_regState <= ReadWaitR;
            end if;

          when ReadWaitA =>
            if pi_nvme_sm.arready = '1' then
              po_nvme_ms.arvalid <= '0';
              s_regReady <= '1';
              s_regState <= Ready;
            end if;

          when ReadWaitR =>
            if pi_nvme_sm.rvalid = '1' then
              po_nvme_ms.rready <= '0';
              s_regRdData <= pi_nvme_sm.rdata;
              s_regReady <= '1';
              s_regState <= Ready;
            end if;

          when WriteWaitAWaitW =>
            if pi_nvme_sm.awready = '1' and pi_nvme_sm.wready = '1' then
              po_nvme_ms.awvalid <= '0';
              po_nvme_ms.wvalid <= '0';
              s_regReady <= '1';
              s_regState <= Ready;
            elsif pi_nvme_sm.awready = '0' and pi_nvme_sm.wready = '1' then
              po_nvme_ms.wvalid <= '0';
              s_regState <= WriteWaitA;
            elsif pi_nvme_sm.awready = '1' and pi_nvme_sm.wready = '0' then
              po_nvme_ms.awvalid <= '0';
              s_regState <= WriteWaitW;
            end if;

          when WriteWaitA =>
            if pi_nvme_sm.awready = '1' then
              po_nvme_ms.awvalid <= '0';
              s_regReady <= '1';
              s_regState <= Ready;
            end if;

          when WriteWaitW =>
            if pi_nvme_sm.wready = '1' then
              po_nvme_ms.wvalid <= '0';
              s_regReady <= '1';
              s_regState <= Ready;
            end if;

          when Ready =>
            s_regReady <= '0';
            s_regState <= Idle;

        end case;
      end if;
    end if;
  end process;

end NvmeController;
