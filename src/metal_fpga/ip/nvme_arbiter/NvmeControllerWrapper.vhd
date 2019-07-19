library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;


use work.fosi_ctrl.all;
use work.fosi_nvme.all;


entity NvmeControllerWrapper is
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

    p_nvme_AWADDR   : out std_logic_vector(31 downto 0);
    p_nvme_AWVALID  : out std_logic;
    p_nvme_WDATA    : out std_logic_vector(31 downto 0);
    p_nvme_WSTRB    : out std_logic_vector(3 downto 0);
    p_nvme_WVALID   : out std_logic;
    p_nvme_BREADY   : out std_logic;
    p_nvme_ARADDR   : out std_logic_vector(31 downto 0);
    p_nvme_ARVALID  : out std_logic;
    p_nvme_RREADY   : out std_logic;
    p_nvme_AWREADY  : in  std_logic;
    p_nvme_WREADY   : in  std_logic;
    p_nvme_BRESP    : in  std_logic_vector(1 downto 0);
    p_nvme_BVALID   : in  std_logic;
    p_nvme_ARREADY  : in  std_logic;
    p_nvme_RDATA    : in  std_logic_vector(31 downto 0);
    p_nvme_RRESP    : in  std_logic_vector(1 downto 0);
    p_nvme_RLAST    : in  std_logic;
    p_nvme_RVALID   : in  std_logic);
end NvmeControllerWrapper;

architecture NvmeControllerWrapper of NvmeControllerWrapper is

signal s_cmdRd_ms : t_NvmeCommandStream_ms;
signal s_cmdRd_sm : t_NvmeCommandStream_sm;
signal s_rspRd_ms : t_NvmeResponseStream_ms;
signal s_rspRd_sm : t_NvmeResponseStream_sm;
signal s_cmdWr_ms : t_NvmeCommandStream_ms;
signal s_cmdWr_sm : t_NvmeCommandStream_sm;
signal s_rspWr_ms : t_NvmeResponseStream_ms;
signal s_rspWr_sm : t_NvmeResponseStream_sm;
signal s_nvme_ms  : t_Ctrl_ms;
signal s_nvme_sm  : t_Ctrl_sm;

begin

  i_controller : entity work.NvmeController
    port map(
      pi_clk      => pi_clk,
      pi_rst_n    => pi_rst_n,
      pi_cmdRd_ms => s_cmdRd_ms,
      po_cmdRd_sm => s_cmdRd_sm,
      po_rspRd_ms => s_rspRd_ms,
      pi_rspRd_sm => s_rspRd_sm,
      pi_cmdWr_ms => s_cmdWr_ms,
      po_cmdWr_sm => s_cmdWr_sm,
      po_rspWr_ms => s_rspWr_ms,
      pi_rspWr_sm => s_rspWr_sm,
      po_nvme_ms  => s_nvme_ms,
      pi_nvme_sm  => s_nvme_sm);

    s_cmdRd_ms.bufferAddr <= unsigned(p_cmdRd_TDATA(
                                           t_NvmeBufferAddr'length-1 downto 0));
    s_cmdRd_ms.blockAddr  <= unsigned(p_cmdRd_TDATA(
                                           t_NvmeBufferAddr'length+
                                           t_NvmeBlockAddr'length-1 downto
                                            t_NvmeBufferAddr'length));
    s_cmdRd_ms.blockCnt   <= unsigned(p_cmdRd_TDATA(
                                           t_NvmeBufferAddr'length+
                                           t_NvmeBlockAddr'length+
                                           t_NvmeBlockCount'length-1 downto
                                            t_NvmeBufferAddr'length+
                                            t_NvmeBlockAddr'length));
    s_cmdRd_ms.drive      <= unsigned(p_cmdRd_TDATA(
                                           t_NvmeBufferAddr'length+
                                           t_NvmeBlockAddr'length+
                                           t_NvmeBlockCount'length+
                                           t_NvmeDriveAddr'length-1 downto
                                            t_NvmeBufferAddr'length+
                                            t_NvmeBlockAddr'length+
                                            t_NvmeBlockCount'length));
    s_cmdRd_ms.valid      <= p_cmdRd_TVALID;
    p_cmdRd_TREADY        <= s_cmdRd_sm.ready;
    p_rspRd_TDATA(0)      <= s_rspRd_ms.status;
    p_rspRd_TVALID        <= s_rspRd_ms.valid;
    s_rspRd_sm.ready      <= p_rspRd_TREADY;

    s_cmdWr_ms.bufferAddr <= unsigned(p_cmdWr_TDATA(
                                           t_NvmeBufferAddr'length-1 downto 0));
    s_cmdWr_ms.blockAddr  <= unsigned(p_cmdWr_TDATA(
                                           t_NvmeBufferAddr'length+
                                           t_NvmeBlockAddr'length-1 downto
                                            t_NvmeBufferAddr'length));
    s_cmdWr_ms.blockCnt   <= unsigned(p_cmdWr_TDATA(
                                           t_NvmeBufferAddr'length+
                                           t_NvmeBlockAddr'length+
                                           t_NvmeBlockCount'length-1 downto
                                            t_NvmeBufferAddr'length+
                                            t_NvmeBlockAddr'length));
    s_cmdWr_ms.drive      <= unsigned(p_cmdWr_TDATA(
                                           t_NvmeBufferAddr'length+
                                           t_NvmeBlockAddr'length+
                                           t_NvmeBlockCount'length+
                                           t_NvmeDriveAddr'length-1 downto
                                            t_NvmeBufferAddr'length+
                                            t_NvmeBlockAddr'length+
                                            t_NvmeBlockCount'length));
    s_cmdWr_ms.valid      <= p_cmdWr_TVALID;
    p_cmdWr_TREADY        <= s_cmdWr_sm.ready;
    p_rspWr_TDATA(0)      <= s_rspWr_ms.status;
    p_rspWr_TVALID        <= s_rspWr_ms.valid;
    s_rspWr_sm.ready      <= p_rspWr_TREADY;

    p_nvme_AWADDR     <= std_logic_vector(s_nvme_ms.awaddr);
    p_nvme_AWVALID    <= s_nvme_ms.awvalid;
    p_nvme_WDATA      <= std_logic_vector(s_nvme_ms.wdata);
    p_nvme_WSTRB      <= std_logic_vector(s_nvme_ms.wstrb);
    p_nvme_WVALID     <= s_nvme_ms.wvalid;
    p_nvme_BREADY     <= s_nvme_ms.bready;
    p_nvme_ARADDR     <= std_logic_vector(s_nvme_ms.araddr);
    p_nvme_ARVALID    <= s_nvme_ms.arvalid;
    p_nvme_RREADY     <= s_nvme_ms.rready;
    s_nvme_sm.awready <= p_nvme_AWREADY;
    s_nvme_sm.wready  <= p_nvme_WREADY;
    s_nvme_sm.bresp   <= unsigned(p_nvme_BRESP);
    s_nvme_sm.bvalid  <= p_nvme_BVALID;
    s_nvme_sm.arready <= p_nvme_ARREADY;
    s_nvme_sm.rdata   <= unsigned(p_nvme_RDATA);
    s_nvme_sm.rresp   <= unsigned(p_nvme_RRESP);
    s_nvme_sm.rlast   <= p_nvme_RLAST;
    s_nvme_sm.rvalid  <= p_nvme_RVALID;

end NvmeControllerWrapper;
