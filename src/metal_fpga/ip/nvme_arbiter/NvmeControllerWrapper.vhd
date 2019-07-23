library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;


use work.fosi_ctrl.all;
use work.fosi_nvme.all;
use work.fosi_util.all;


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

    p_nvme_araddr          : out std_logic_vector ( 31 downto 0 );
    p_nvme_arburst         : out std_logic_vector ( 1 downto 0 );
    p_nvme_arcache         : out std_logic_vector ( 3 downto 0 );
    p_nvme_arid            : out std_logic_vector ( 0 downto 0 );
    p_nvme_arlen           : out std_logic_vector ( 7 downto 0 );
    p_nvme_arlock          : out std_logic_vector ( 1 downto 0 );
    p_nvme_arprot          : out std_logic_vector ( 2 downto 0 );
    p_nvme_arqos           : out std_logic_vector ( 3 downto 0 );
    p_nvme_arready         : in  std_logic;
    p_nvme_arregion        : out std_logic_vector ( 3 downto 0 );
    p_nvme_arsize          : out std_logic_vector ( 2 downto 0 );
    p_nvme_aruser          : out std_logic_vector ( 0 downto 0 );
    p_nvme_arvalid         : out std_logic;
    p_nvme_awaddr          : out std_logic_vector ( 31 downto 0 );
    p_nvme_awburst         : out std_logic_vector ( 1 downto 0 );
    p_nvme_awcache         : out std_logic_vector ( 3 downto 0 );
    p_nvme_awid            : out std_logic_vector ( 0 downto 0 );
    p_nvme_awlen           : out std_logic_vector ( 7 downto 0 );
    p_nvme_awlock          : out std_logic_vector ( 1 downto 0 );
    p_nvme_awprot          : out std_logic_vector ( 2 downto 0 );
    p_nvme_awqos           : out std_logic_vector ( 3 downto 0 );
    p_nvme_awready         : in  std_logic;
    p_nvme_awregion        : out std_logic_vector ( 3 downto 0 );
    p_nvme_awsize          : out std_logic_vector ( 2 downto 0 );
    p_nvme_awuser          : out std_logic_vector ( 0 downto 0 );
    p_nvme_awvalid         : out std_logic;
    p_nvme_bid             : in  std_logic_vector ( 0 downto 0 );
    p_nvme_bready          : out std_logic;
    p_nvme_bresp           : in  std_logic_vector ( 1 downto 0 );
    p_nvme_buser           : in  std_logic_vector ( 0 downto 0 );
    p_nvme_bvalid          : in  std_logic;
    p_nvme_rdata           : in  std_logic_vector ( 31 downto 0 );
    p_nvme_rid             : in  std_logic_vector ( 0 downto 0 );
    p_nvme_rlast           : in  std_logic;
    p_nvme_rready          : out std_logic;
    p_nvme_rresp           : in  std_logic_vector ( 1 downto 0 );
    p_nvme_ruser           : in  std_logic_vector ( 0 downto 0 );
    p_nvme_rvalid          : in  std_logic;
    p_nvme_wdata           : out std_logic_vector (31 downto 0 );
    p_nvme_wlast           : out std_logic;
    p_nvme_wready          : in  std_logic;
    p_nvme_wstrb           : out std_logic_vector (3 downto 0 );
    p_nvme_wuser           : out std_logic_vector (0 downto 0 );
    p_nvme_wvalid          : out std_logic);
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

    p_nvme_awid     <=           (others => '0');
    p_nvme_awaddr   <= f_resizeUV(s_nvme_ms.awaddr, p_nvme_awaddr'length);
    p_nvme_awlen    <= f_resizeUV(c_AxiNullLen, p_nvme_awlen'length);
    p_nvme_awsize   <= f_resizeUV(c_CtrlFullSize, p_nvme_awsize'length);
    p_nvme_awburst  <= f_resizeUV(c_AxiNullBurst, p_nvme_awburst'length);
    p_nvme_awlock   <= f_resizeUV(c_AxiNullLock, p_nvme_awlock'length);
    p_nvme_awcache  <= f_resizeUV(c_AxiNullCache, p_nvme_awcache'length);
    p_nvme_awprot   <= f_resizeUV(c_AxiNullProt, p_nvme_awprot'length);
    p_nvme_awqos    <= f_resizeUV(c_AxiNullQos, p_nvme_awqos'length);
    p_nvme_awregion <= f_resizeUV(c_AxiNullRegion, p_nvme_awregion'length);
    p_nvme_awvalid  <=            s_nvme_ms.awvalid;
    p_nvme_wdata    <= f_resizeUV(s_nvme_ms.wdata, p_nvme_wdata'length);
    p_nvme_wstrb    <= f_resizeUV(s_nvme_ms.wstrb, p_nvme_wstrb'length);
    p_nvme_wlast    <=            '1';                                        -- No write bursts, so wlast always '1'
    p_nvme_wuser    <=           (others => '0');
    p_nvme_wvalid   <=            s_nvme_ms.wvalid;
    p_nvme_bready   <=            s_nvme_ms.bready;
    p_nvme_arid     <=           (others => '0');
    p_nvme_araddr   <= f_resizeUV(s_nvme_ms.araddr, p_nvme_araddr'length);
    p_nvme_arlen    <= f_resizeUV(c_AxiNullLen, p_nvme_arlen'length);
    p_nvme_arsize   <= f_resizeUV(c_CtrlFullSize, p_nvme_arsize'length);
    p_nvme_arburst  <= f_resizeUV(c_AxiNullBurst, p_nvme_arburst'length);
    p_nvme_arlock   <= f_resizeUV(c_AxiNullLock, p_nvme_arlock'length);
    p_nvme_arcache  <= f_resizeUV(c_AxiNullCache, p_nvme_arcache'length);
    p_nvme_arprot   <= f_resizeUV(c_AxiNullProt, p_nvme_arprot'length);
    p_nvme_arqos    <= f_resizeUV(c_AxiNullQos, p_nvme_arqos'length);
    p_nvme_arregion <= f_resizeUV(c_AxiNullRegion, p_nvme_arregion'length);
    p_nvme_arvalid  <=            s_nvme_ms.arvalid;
    p_nvme_rready   <=            s_nvme_ms.rready;
    s_nvme_sm.awready   <=            p_nvme_awready;
    s_nvme_sm.wready    <=            p_nvme_wready;
    s_nvme_sm.bresp     <= f_resizeVU(p_nvme_bresp, s_nvme_sm.bresp'length);
    s_nvme_sm.bvalid    <=            p_nvme_bvalid;
    s_nvme_sm.arready   <=            p_nvme_arready;
    s_nvme_sm.rdata     <= f_resizeVU(p_nvme_rdata, s_nvme_sm.rdata'length);
    s_nvme_sm.rresp     <= f_resizeVU(p_nvme_rresp, s_nvme_sm.rresp'length);
    s_nvme_sm.rlast     <=            p_nvme_rlast;
    s_nvme_sm.rvalid    <=            p_nvme_rvalid;

end NvmeControllerWrapper;
