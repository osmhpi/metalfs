/*
 * Copyright 2019 International Business Machines
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
`include "snap_global_vars.v"

module action_wrapper (
   input                                   ap_clk                   ,
   input                                   ap_rst_n                 ,
   output                                  interrupt                ,
   output [`INT_BITS-1:0]                  interrupt_src            ,
   output [`CTXW-1:0]                      interrupt_ctx            ,
   input                                   interrupt_ack            ,
    //
    // AXI Control Register inputterface
   input [ `AXI_LITE_AW-1 : 0]             s_axi_ctrl_reg_araddr    ,
   output                                  s_axi_ctrl_reg_arready   ,
   input                                   s_axi_ctrl_reg_arvalid   ,
   input [ `AXI_LITE_AW-1 : 0]             s_axi_ctrl_reg_awaddr    ,
   output                                  s_axi_ctrl_reg_awready   ,
   input                                   s_axi_ctrl_reg_awvalid   ,
   input                                   s_axi_ctrl_reg_bready    ,
   output [ 1 : 0]                         s_axi_ctrl_reg_bresp     ,
   output                                  s_axi_ctrl_reg_bvalid    ,
   output [ `AXI_LITE_DW-1 : 0]            s_axi_ctrl_reg_rdata     ,
   input                                   s_axi_ctrl_reg_rready    ,
   output [ 1 : 0]                         s_axi_ctrl_reg_rresp     ,
   output                                  s_axi_ctrl_reg_rvalid    ,
   input [ `AXI_LITE_DW-1 : 0]             s_axi_ctrl_reg_wdata     ,
   output                                  s_axi_ctrl_reg_wready    ,
   input [(`AXI_LITE_DW/8)-1 : 0]          s_axi_ctrl_reg_wstrb     ,
   input                                   s_axi_ctrl_reg_wvalid    ,
`ifdef ENABLE_AXI_CARD_MEM
`ifndef ENABLE_HBM
   output [ `AXI_CARD_MEM_ADDR_WIDTH-1 : 0]  m_axi_card_mem0_araddr   ,
   output [ 1 : 0]                         m_axi_card_mem0_arburst  ,
   output [ 3 : 0]                         m_axi_card_mem0_arcache  ,
   output [ `AXI_CARD_MEM_ID_WIDTH-1 : 0]    m_axi_card_mem0_arid     ,
   output [ 7 : 0]                         m_axi_card_mem0_arlen    ,
   output [ 1 : 0]                         m_axi_card_mem0_arlock   ,
   output [ 2 : 0]                         m_axi_card_mem0_arprot   ,
   output [ 3 : 0]                         m_axi_card_mem0_arqos    ,
   input                                   m_axi_card_mem0_arready  ,
   output [ 3 : 0]                         m_axi_card_mem0_arregion ,
   output [ 2 : 0]                         m_axi_card_mem0_arsize   ,
   output [ `AXI_CARD_MEM_USER_WIDTH-1 : 0] m_axi_card_mem0_aruser  ,
   output                                  m_axi_card_mem0_arvalid  ,
   output [ `AXI_CARD_MEM_ADDR_WIDTH-1 : 0]  m_axi_card_mem0_awaddr   ,
   output [ 1 : 0]                         m_axi_card_mem0_awburst  ,
   output [ 3 : 0]                         m_axi_card_mem0_awcache  ,
   output [ `AXI_CARD_MEM_ID_WIDTH-1 : 0]    m_axi_card_mem0_awid     ,
   output [ 7 : 0]                         m_axi_card_mem0_awlen    ,
   output [ 1 : 0]                         m_axi_card_mem0_awlock   ,
   output [ 2 : 0]                         m_axi_card_mem0_awprot   ,
   output [ 3 : 0]                         m_axi_card_mem0_awqos    ,
   input                                   m_axi_card_mem0_awready  ,
   output [ 3 : 0]                         m_axi_card_mem0_awregion ,
   output [ 2 : 0]                         m_axi_card_mem0_awsize   ,
   output [ `AXI_CARD_MEM_USER_WIDTH-1 : 0] m_axi_card_mem0_awuser  ,
   output                                  m_axi_card_mem0_awvalid  ,
   input [`AXI_CARD_MEM_ID_WIDTH-1 : 0]     m_axi_card_mem0_bid      ,
   output                                  m_axi_card_mem0_bready   ,
   input [ 1 : 0]                          m_axi_card_mem0_bresp    ,
   input [`AXI_CARD_MEM_USER_WIDTH-1 : 0]  m_axi_card_mem0_buser    ,
   input                                   m_axi_card_mem0_bvalid   ,
   input [`AXI_CARD_MEM_DATA_WIDTH-1 : 0]   m_axi_card_mem0_rdata    ,
   input [`AXI_CARD_MEM_ID_WIDTH-1 : 0]     m_axi_card_mem0_rid      ,
   input                                   m_axi_card_mem0_rlast    ,
   output                                  m_axi_card_mem0_rready   ,
   input [ 1 : 0]                          m_axi_card_mem0_rresp    ,
   input [ `AXI_CARD_MEM_USER_WIDTH-1 : 0] m_axi_card_mem0_ruser    ,
   input                                   m_axi_card_mem0_rvalid   ,
   output [`AXI_CARD_MEM_DATA_WIDTH-1 : 0]  m_axi_card_mem0_wdata    ,
   output                                  m_axi_card_mem0_wlast    ,
   input                                   m_axi_card_mem0_wready   ,
   output [(`AXI_CARD_MEM_DATA_WIDTH/8)-1 : 0] m_axi_card_mem0_wstrb  ,
   output [`AXI_CARD_MEM_USER_WIDTH-1 : 0] m_axi_card_mem0_wuser    ,
   output                                  m_axi_card_mem0_wvalid   ,

`else
   `ifdef HBM_AXI_IF_P0
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p0_araddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p0_arburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p0_arcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p0_arid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p0_arlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p0_arlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p0_arprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p0_arqos    ,
   input                                   m_axi_card_hbm_p0_arready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p0_arregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p0_arsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p0_aruser  ,
   output                                  m_axi_card_hbm_p0_arvalid  ,
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p0_awaddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p0_awburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p0_awcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p0_awid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p0_awlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p0_awlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p0_awprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p0_awqos    ,
   input                                   m_axi_card_hbm_p0_awready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p0_awregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p0_awsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p0_awuser  ,
   output                                  m_axi_card_hbm_p0_awvalid  ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p0_bid      ,
   output                                  m_axi_card_hbm_p0_bready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p0_bresp    ,
   input [`AXI_CARD_HBM_USER_WIDTH-1 : 0]  m_axi_card_hbm_p0_buser    ,
   input                                   m_axi_card_hbm_p0_bvalid   ,
   input [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]   m_axi_card_hbm_p0_rdata    ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p0_rid      ,
   input                                   m_axi_card_hbm_p0_rlast    ,
   output                                  m_axi_card_hbm_p0_rready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p0_rresp    ,
   input [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p0_ruser    ,
   input                                   m_axi_card_hbm_p0_rvalid   ,
   output [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]  m_axi_card_hbm_p0_wdata    ,
   output                                  m_axi_card_hbm_p0_wlast    ,
   input                                   m_axi_card_hbm_p0_wready   ,
   output [(`AXI_CARD_HBM_DATA_WIDTH/8)-1 : 0] m_axi_card_hbm_p0_wstrb  ,
   output [`AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p0_wuser    ,
   output                                  m_axi_card_hbm_p0_wvalid   ,
   `endif

   `ifdef HBM_AXI_IF_P1
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p1_araddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p1_arburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p1_arcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p1_arid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p1_arlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p1_arlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p1_arprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p1_arqos    ,
   input                                   m_axi_card_hbm_p1_arready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p1_arregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p1_arsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p1_aruser  ,
   output                                  m_axi_card_hbm_p1_arvalid  ,
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p1_awaddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p1_awburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p1_awcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p1_awid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p1_awlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p1_awlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p1_awprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p1_awqos    ,
   input                                   m_axi_card_hbm_p1_awready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p1_awregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p1_awsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p1_awuser  ,
   output                                  m_axi_card_hbm_p1_awvalid  ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p1_bid      ,
   output                                  m_axi_card_hbm_p1_bready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p1_bresp    ,
   input [`AXI_CARD_HBM_USER_WIDTH-1 : 0]  m_axi_card_hbm_p1_buser    ,
   input                                   m_axi_card_hbm_p1_bvalid   ,
   input [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]   m_axi_card_hbm_p1_rdata    ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p1_rid      ,
   input                                   m_axi_card_hbm_p1_rlast    ,
   output                                  m_axi_card_hbm_p1_rready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p1_rresp    ,
   input [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p1_ruser    ,
   input                                   m_axi_card_hbm_p1_rvalid   ,
   output [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]  m_axi_card_hbm_p1_wdata    ,
   output                                  m_axi_card_hbm_p1_wlast    ,
   input                                   m_axi_card_hbm_p1_wready   ,
   output [(`AXI_CARD_HBM_DATA_WIDTH/8)-1 : 0] m_axi_card_hbm_p1_wstrb  ,
   output [`AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p1_wuser    ,
   output                                  m_axi_card_hbm_p1_wvalid   ,
   `endif

   `ifdef HBM_AXI_IF_P2
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p2_araddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p2_arburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p2_arcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p2_arid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p2_arlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p2_arlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p2_arprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p2_arqos    ,
   input                                   m_axi_card_hbm_p2_arready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p2_arregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p2_arsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p2_aruser  ,
   output                                  m_axi_card_hbm_p2_arvalid  ,
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p2_awaddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p2_awburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p2_awcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p2_awid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p2_awlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p2_awlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p2_awprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p2_awqos    ,
   input                                   m_axi_card_hbm_p2_awready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p2_awregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p2_awsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p2_awuser  ,
   output                                  m_axi_card_hbm_p2_awvalid  ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p2_bid      ,
   output                                  m_axi_card_hbm_p2_bready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p2_bresp    ,
   input [`AXI_CARD_HBM_USER_WIDTH-1 : 0]  m_axi_card_hbm_p2_buser    ,
   input                                   m_axi_card_hbm_p2_bvalid   ,
   input [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]   m_axi_card_hbm_p2_rdata    ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p2_rid      ,
   input                                   m_axi_card_hbm_p2_rlast    ,
   output                                  m_axi_card_hbm_p2_rready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p2_rresp    ,
   input [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p2_ruser    ,
   input                                   m_axi_card_hbm_p2_rvalid   ,
   output [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]  m_axi_card_hbm_p2_wdata    ,
   output                                  m_axi_card_hbm_p2_wlast    ,
   input                                   m_axi_card_hbm_p2_wready   ,
   output [(`AXI_CARD_HBM_DATA_WIDTH/8)-1 : 0] m_axi_card_hbm_p2_wstrb  ,
   output [`AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p2_wuser    ,
   output                                  m_axi_card_hbm_p2_wvalid   ,
   `endif

   `ifdef HBM_AXI_IF_P3
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p3_araddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p3_arburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p3_arcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p3_arid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p3_arlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p3_arlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p3_arprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p3_arqos    ,
   input                                   m_axi_card_hbm_p3_arready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p3_arregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p3_arsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p3_aruser  ,
   output                                  m_axi_card_hbm_p3_arvalid  ,
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p3_awaddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p3_awburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p3_awcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p3_awid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p3_awlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p3_awlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p3_awprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p3_awqos    ,
   input                                   m_axi_card_hbm_p3_awready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p3_awregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p3_awsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p3_awuser  ,
   output                                  m_axi_card_hbm_p3_awvalid  ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p3_bid      ,
   output                                  m_axi_card_hbm_p3_bready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p3_bresp    ,
   input [`AXI_CARD_HBM_USER_WIDTH-1 : 0]  m_axi_card_hbm_p3_buser    ,
   input                                   m_axi_card_hbm_p3_bvalid   ,
   input [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]   m_axi_card_hbm_p3_rdata    ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p3_rid      ,
   input                                   m_axi_card_hbm_p3_rlast    ,
   output                                  m_axi_card_hbm_p3_rready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p3_rresp    ,
   input [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p3_ruser    ,
   input                                   m_axi_card_hbm_p3_rvalid   ,
   output [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]  m_axi_card_hbm_p3_wdata    ,
   output                                  m_axi_card_hbm_p3_wlast    ,
   input                                   m_axi_card_hbm_p3_wready   ,
   output [(`AXI_CARD_HBM_DATA_WIDTH/8)-1 : 0] m_axi_card_hbm_p3_wstrb  ,
   output [`AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p3_wuser    ,
   output                                  m_axi_card_hbm_p3_wvalid   ,
   `endif

   `ifdef HBM_AXI_IF_P4
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p4_araddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p4_arburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p4_arcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p4_arid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p4_arlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p4_arlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p4_arprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p4_arqos    ,
   input                                   m_axi_card_hbm_p4_arready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p4_arregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p4_arsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p4_aruser  ,
   output                                  m_axi_card_hbm_p4_arvalid  ,
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p4_awaddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p4_awburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p4_awcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p4_awid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p4_awlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p4_awlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p4_awprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p4_awqos    ,
   input                                   m_axi_card_hbm_p4_awready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p4_awregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p4_awsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p4_awuser  ,
   output                                  m_axi_card_hbm_p4_awvalid  ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p4_bid      ,
   output                                  m_axi_card_hbm_p4_bready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p4_bresp    ,
   input [`AXI_CARD_HBM_USER_WIDTH-1 : 0]  m_axi_card_hbm_p4_buser    ,
   input                                   m_axi_card_hbm_p4_bvalid   ,
   input [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]   m_axi_card_hbm_p4_rdata    ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p4_rid      ,
   input                                   m_axi_card_hbm_p4_rlast    ,
   output                                  m_axi_card_hbm_p4_rready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p4_rresp    ,
   input [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p4_ruser    ,
   input                                   m_axi_card_hbm_p4_rvalid   ,
   output [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]  m_axi_card_hbm_p4_wdata    ,
   output                                  m_axi_card_hbm_p4_wlast    ,
   input                                   m_axi_card_hbm_p4_wready   ,
   output [(`AXI_CARD_HBM_DATA_WIDTH/8)-1 : 0] m_axi_card_hbm_p4_wstrb  ,
   output [`AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p4_wuser    ,
   output                                  m_axi_card_hbm_p4_wvalid   ,
   `endif

   `ifdef HBM_AXI_IF_P5
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p5_araddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p5_arburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p5_arcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p5_arid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p5_arlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p5_arlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p5_arprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p5_arqos    ,
   input                                   m_axi_card_hbm_p5_arready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p5_arregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p5_arsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p5_aruser  ,
   output                                  m_axi_card_hbm_p5_arvalid  ,
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p5_awaddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p5_awburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p5_awcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p5_awid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p5_awlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p5_awlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p5_awprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p5_awqos    ,
   input                                   m_axi_card_hbm_p5_awready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p5_awregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p5_awsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p5_awuser  ,
   output                                  m_axi_card_hbm_p5_awvalid  ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p5_bid      ,
   output                                  m_axi_card_hbm_p5_bready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p5_bresp    ,
   input [`AXI_CARD_HBM_USER_WIDTH-1 : 0]  m_axi_card_hbm_p5_buser    ,
   input                                   m_axi_card_hbm_p5_bvalid   ,
   input [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]   m_axi_card_hbm_p5_rdata    ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p5_rid      ,
   input                                   m_axi_card_hbm_p5_rlast    ,
   output                                  m_axi_card_hbm_p5_rready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p5_rresp    ,
   input [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p5_ruser    ,
   input                                   m_axi_card_hbm_p5_rvalid   ,
   output [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]  m_axi_card_hbm_p5_wdata    ,
   output                                  m_axi_card_hbm_p5_wlast    ,
   input                                   m_axi_card_hbm_p5_wready   ,
   output [(`AXI_CARD_HBM_DATA_WIDTH/8)-1 : 0] m_axi_card_hbm_p5_wstrb  ,
   output [`AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p5_wuser    ,
   output                                  m_axi_card_hbm_p5_wvalid   ,
   `endif

   `ifdef HBM_AXI_IF_P6
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p6_araddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p6_arburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p6_arcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p6_arid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p6_arlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p6_arlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p6_arprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p6_arqos    ,
   input                                   m_axi_card_hbm_p6_arready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p6_arregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p6_arsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p6_aruser  ,
   output                                  m_axi_card_hbm_p6_arvalid  ,
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p6_awaddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p6_awburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p6_awcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p6_awid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p6_awlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p6_awlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p6_awprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p6_awqos    ,
   input                                   m_axi_card_hbm_p6_awready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p6_awregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p6_awsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p6_awuser  ,
   output                                  m_axi_card_hbm_p6_awvalid  ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p6_bid      ,
   output                                  m_axi_card_hbm_p6_bready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p6_bresp    ,
   input [`AXI_CARD_HBM_USER_WIDTH-1 : 0]  m_axi_card_hbm_p6_buser    ,
   input                                   m_axi_card_hbm_p6_bvalid   ,
   input [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]   m_axi_card_hbm_p6_rdata    ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p6_rid      ,
   input                                   m_axi_card_hbm_p6_rlast    ,
   output                                  m_axi_card_hbm_p6_rready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p6_rresp    ,
   input [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p6_ruser    ,
   input                                   m_axi_card_hbm_p6_rvalid   ,
   output [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]  m_axi_card_hbm_p6_wdata    ,
   output                                  m_axi_card_hbm_p6_wlast    ,
   input                                   m_axi_card_hbm_p6_wready   ,
   output [(`AXI_CARD_HBM_DATA_WIDTH/8)-1 : 0] m_axi_card_hbm_p6_wstrb  ,
   output [`AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p6_wuser    ,
   output                                  m_axi_card_hbm_p6_wvalid   ,
   `endif

   `ifdef HBM_AXI_IF_P7
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p7_araddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p7_arburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p7_arcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p7_arid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p7_arlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p7_arlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p7_arprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p7_arqos    ,
   input                                   m_axi_card_hbm_p7_arready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p7_arregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p7_arsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p7_aruser  ,
   output                                  m_axi_card_hbm_p7_arvalid  ,
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p7_awaddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p7_awburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p7_awcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p7_awid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p7_awlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p7_awlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p7_awprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p7_awqos    ,
   input                                   m_axi_card_hbm_p7_awready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p7_awregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p7_awsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p7_awuser  ,
   output                                  m_axi_card_hbm_p7_awvalid  ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p7_bid      ,
   output                                  m_axi_card_hbm_p7_bready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p7_bresp    ,
   input [`AXI_CARD_HBM_USER_WIDTH-1 : 0]  m_axi_card_hbm_p7_buser    ,
   input                                   m_axi_card_hbm_p7_bvalid   ,
   input [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]   m_axi_card_hbm_p7_rdata    ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p7_rid      ,
   input                                   m_axi_card_hbm_p7_rlast    ,
   output                                  m_axi_card_hbm_p7_rready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p7_rresp    ,
   input [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p7_ruser    ,
   input                                   m_axi_card_hbm_p7_rvalid   ,
   output [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]  m_axi_card_hbm_p7_wdata    ,
   output                                  m_axi_card_hbm_p7_wlast    ,
   input                                   m_axi_card_hbm_p7_wready   ,
   output [(`AXI_CARD_HBM_DATA_WIDTH/8)-1 : 0] m_axi_card_hbm_p7_wstrb  ,
   output [`AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p7_wuser    ,
   output                                  m_axi_card_hbm_p7_wvalid   ,
   `endif

   `ifdef HBM_AXI_IF_P8
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p8_araddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p8_arburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p8_arcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p8_arid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p8_arlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p8_arlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p8_arprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p8_arqos    ,
   input                                   m_axi_card_hbm_p8_arready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p8_arregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p8_arsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p8_aruser  ,
   output                                  m_axi_card_hbm_p8_arvalid  ,
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p8_awaddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p8_awburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p8_awcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p8_awid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p8_awlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p8_awlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p8_awprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p8_awqos    ,
   input                                   m_axi_card_hbm_p8_awready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p8_awregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p8_awsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p8_awuser  ,
   output                                  m_axi_card_hbm_p8_awvalid  ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p8_bid      ,
   output                                  m_axi_card_hbm_p8_bready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p8_bresp    ,
   input [`AXI_CARD_HBM_USER_WIDTH-1 : 0]  m_axi_card_hbm_p8_buser    ,
   input                                   m_axi_card_hbm_p8_bvalid   ,
   input [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]   m_axi_card_hbm_p8_rdata    ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p8_rid      ,
   input                                   m_axi_card_hbm_p8_rlast    ,
   output                                  m_axi_card_hbm_p8_rready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p8_rresp    ,
   input [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p8_ruser    ,
   input                                   m_axi_card_hbm_p8_rvalid   ,
   output [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]  m_axi_card_hbm_p8_wdata    ,
   output                                  m_axi_card_hbm_p8_wlast    ,
   input                                   m_axi_card_hbm_p8_wready   ,
   output [(`AXI_CARD_HBM_DATA_WIDTH/8)-1 : 0] m_axi_card_hbm_p8_wstrb  ,
   output [`AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p8_wuser    ,
   output                                  m_axi_card_hbm_p8_wvalid   ,
   `endif

   `ifdef HBM_AXI_IF_P9
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p9_araddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p9_arburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p9_arcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p9_arid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p9_arlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p9_arlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p9_arprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p9_arqos    ,
   input                                   m_axi_card_hbm_p9_arready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p9_arregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p9_arsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p9_aruser  ,
   output                                  m_axi_card_hbm_p9_arvalid  ,
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p9_awaddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p9_awburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p9_awcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p9_awid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p9_awlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p9_awlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p9_awprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p9_awqos    ,
   input                                   m_axi_card_hbm_p9_awready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p9_awregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p9_awsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p9_awuser  ,
   output                                  m_axi_card_hbm_p9_awvalid  ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p9_bid      ,
   output                                  m_axi_card_hbm_p9_bready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p9_bresp    ,
   input [`AXI_CARD_HBM_USER_WIDTH-1 : 0]  m_axi_card_hbm_p9_buser    ,
   input                                   m_axi_card_hbm_p9_bvalid   ,
   input [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]   m_axi_card_hbm_p9_rdata    ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p9_rid      ,
   input                                   m_axi_card_hbm_p9_rlast    ,
   output                                  m_axi_card_hbm_p9_rready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p9_rresp    ,
   input [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p9_ruser    ,
   input                                   m_axi_card_hbm_p9_rvalid   ,
   output [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]  m_axi_card_hbm_p9_wdata    ,
   output                                  m_axi_card_hbm_p9_wlast    ,
   input                                   m_axi_card_hbm_p9_wready   ,
   output [(`AXI_CARD_HBM_DATA_WIDTH/8)-1 : 0] m_axi_card_hbm_p9_wstrb  ,
   output [`AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p9_wuser    ,
   output                                  m_axi_card_hbm_p9_wvalid   ,
   `endif

   `ifdef HBM_AXI_IF_P10
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p10_araddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p10_arburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p10_arcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p10_arid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p10_arlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p10_arlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p10_arprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p10_arqos    ,
   input                                   m_axi_card_hbm_p10_arready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p10_arregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p10_arsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p10_aruser  ,
   output                                  m_axi_card_hbm_p10_arvalid  ,
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p10_awaddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p10_awburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p10_awcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p10_awid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p10_awlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p10_awlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p10_awprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p10_awqos    ,
   input                                   m_axi_card_hbm_p10_awready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p10_awregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p10_awsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p10_awuser  ,
   output                                  m_axi_card_hbm_p10_awvalid  ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p10_bid      ,
   output                                  m_axi_card_hbm_p10_bready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p10_bresp    ,
   input [`AXI_CARD_HBM_USER_WIDTH-1 : 0]  m_axi_card_hbm_p10_buser    ,
   input                                   m_axi_card_hbm_p10_bvalid   ,
   input [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]   m_axi_card_hbm_p10_rdata    ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p10_rid      ,
   input                                   m_axi_card_hbm_p10_rlast    ,
   output                                  m_axi_card_hbm_p10_rready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p10_rresp    ,
   input [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p10_ruser    ,
   input                                   m_axi_card_hbm_p10_rvalid   ,
   output [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]  m_axi_card_hbm_p10_wdata    ,
   output                                  m_axi_card_hbm_p10_wlast    ,
   input                                   m_axi_card_hbm_p10_wready   ,
   output [(`AXI_CARD_HBM_DATA_WIDTH/8)-1 : 0] m_axi_card_hbm_p10_wstrb  ,
   output [`AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p10_wuser    ,
   output                                  m_axi_card_hbm_p10_wvalid   ,
   `endif

   `ifdef HBM_AXI_IF_P11
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p11_araddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p11_arburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p11_arcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p11_arid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p11_arlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p11_arlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p11_arprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p11_arqos    ,
   input                                   m_axi_card_hbm_p11_arready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p11_arregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p11_arsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p11_aruser  ,
   output                                  m_axi_card_hbm_p11_arvalid  ,
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p11_awaddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p11_awburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p11_awcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p11_awid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p11_awlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p11_awlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p11_awprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p11_awqos    ,
   input                                   m_axi_card_hbm_p11_awready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p11_awregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p11_awsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p11_awuser  ,
   output                                  m_axi_card_hbm_p11_awvalid  ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p11_bid      ,
   output                                  m_axi_card_hbm_p11_bready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p11_bresp    ,
   input [`AXI_CARD_HBM_USER_WIDTH-1 : 0]  m_axi_card_hbm_p11_buser    ,
   input                                   m_axi_card_hbm_p11_bvalid   ,
   input [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]   m_axi_card_hbm_p11_rdata    ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p11_rid      ,
   input                                   m_axi_card_hbm_p11_rlast    ,
   output                                  m_axi_card_hbm_p11_rready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p11_rresp    ,
   input [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p11_ruser    ,
   input                                   m_axi_card_hbm_p11_rvalid   ,
   output [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]  m_axi_card_hbm_p11_wdata    ,
   output                                  m_axi_card_hbm_p11_wlast    ,
   input                                   m_axi_card_hbm_p11_wready   ,
   output [(`AXI_CARD_HBM_DATA_WIDTH/8)-1 : 0] m_axi_card_hbm_p11_wstrb  ,
   output [`AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p11_wuser    ,
   output                                  m_axi_card_hbm_p11_wvalid   ,
   `endif

   `ifdef HBM_AXI_IF_P12
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p12_araddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p12_arburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p12_arcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p12_arid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p12_arlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p12_arlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p12_arprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p12_arqos    ,
   input                                   m_axi_card_hbm_p12_arready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p12_arregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p12_arsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p12_aruser  ,
   output                                  m_axi_card_hbm_p12_arvalid  ,
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p12_awaddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p12_awburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p12_awcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p12_awid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p12_awlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p12_awlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p12_awprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p12_awqos    ,
   input                                   m_axi_card_hbm_p12_awready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p12_awregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p12_awsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p12_awuser  ,
   output                                  m_axi_card_hbm_p12_awvalid  ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p12_bid      ,
   output                                  m_axi_card_hbm_p12_bready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p12_bresp    ,
   input [`AXI_CARD_HBM_USER_WIDTH-1 : 0]  m_axi_card_hbm_p12_buser    ,
   input                                   m_axi_card_hbm_p12_bvalid   ,
   input [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]   m_axi_card_hbm_p12_rdata    ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p12_rid      ,
   input                                   m_axi_card_hbm_p12_rlast    ,
   output                                  m_axi_card_hbm_p12_rready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p12_rresp    ,
   input [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p12_ruser    ,
   input                                   m_axi_card_hbm_p12_rvalid   ,
   output [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]  m_axi_card_hbm_p12_wdata    ,
   output                                  m_axi_card_hbm_p12_wlast    ,
   input                                   m_axi_card_hbm_p12_wready   ,
   output [(`AXI_CARD_HBM_DATA_WIDTH/8)-1 : 0] m_axi_card_hbm_p12_wstrb  ,
   output [`AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p12_wuser    ,
   output                                  m_axi_card_hbm_p12_wvalid   ,
   `endif

   `ifdef HBM_AXI_IF_P13
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p13_araddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p13_arburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p13_arcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p13_arid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p13_arlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p13_arlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p13_arprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p13_arqos    ,
   input                                   m_axi_card_hbm_p13_arready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p13_arregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p13_arsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p13_aruser  ,
   output                                  m_axi_card_hbm_p13_arvalid  ,
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p13_awaddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p13_awburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p13_awcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p13_awid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p13_awlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p13_awlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p13_awprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p13_awqos    ,
   input                                   m_axi_card_hbm_p13_awready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p13_awregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p13_awsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p13_awuser  ,
   output                                  m_axi_card_hbm_p13_awvalid  ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p13_bid      ,
   output                                  m_axi_card_hbm_p13_bready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p13_bresp    ,
   input [`AXI_CARD_HBM_USER_WIDTH-1 : 0]  m_axi_card_hbm_p13_buser    ,
   input                                   m_axi_card_hbm_p13_bvalid   ,
   input [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]   m_axi_card_hbm_p13_rdata    ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p13_rid      ,
   input                                   m_axi_card_hbm_p13_rlast    ,
   output                                  m_axi_card_hbm_p13_rready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p13_rresp    ,
   input [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p13_ruser    ,
   input                                   m_axi_card_hbm_p13_rvalid   ,
   output [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]  m_axi_card_hbm_p13_wdata    ,
   output                                  m_axi_card_hbm_p13_wlast    ,
   input                                   m_axi_card_hbm_p13_wready   ,
   output [(`AXI_CARD_HBM_DATA_WIDTH/8)-1 : 0] m_axi_card_hbm_p13_wstrb  ,
   output [`AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p13_wuser    ,
   output                                  m_axi_card_hbm_p13_wvalid   ,
   `endif

   `ifdef HBM_AXI_IF_P14
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p14_araddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p14_arburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p14_arcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p14_arid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p14_arlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p14_arlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p14_arprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p14_arqos    ,
   input                                   m_axi_card_hbm_p14_arready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p14_arregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p14_arsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p14_aruser  ,
   output                                  m_axi_card_hbm_p14_arvalid  ,
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p14_awaddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p14_awburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p14_awcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p14_awid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p14_awlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p14_awlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p14_awprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p14_awqos    ,
   input                                   m_axi_card_hbm_p14_awready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p14_awregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p14_awsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p14_awuser  ,
   output                                  m_axi_card_hbm_p14_awvalid  ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p14_bid      ,
   output                                  m_axi_card_hbm_p14_bready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p14_bresp    ,
   input [`AXI_CARD_HBM_USER_WIDTH-1 : 0]  m_axi_card_hbm_p14_buser    ,
   input                                   m_axi_card_hbm_p14_bvalid   ,
   input [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]   m_axi_card_hbm_p14_rdata    ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p14_rid      ,
   input                                   m_axi_card_hbm_p14_rlast    ,
   output                                  m_axi_card_hbm_p14_rready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p14_rresp    ,
   input [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p14_ruser    ,
   input                                   m_axi_card_hbm_p14_rvalid   ,
   output [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]  m_axi_card_hbm_p14_wdata    ,
   output                                  m_axi_card_hbm_p14_wlast    ,
   input                                   m_axi_card_hbm_p14_wready   ,
   output [(`AXI_CARD_HBM_DATA_WIDTH/8)-1 : 0] m_axi_card_hbm_p14_wstrb  ,
   output [`AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p14_wuser    ,
   output                                  m_axi_card_hbm_p14_wvalid   ,
   `endif

   `ifdef HBM_AXI_IF_P15
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p15_araddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p15_arburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p15_arcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p15_arid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p15_arlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p15_arlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p15_arprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p15_arqos    ,
   input                                   m_axi_card_hbm_p15_arready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p15_arregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p15_arsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p15_aruser  ,
   output                                  m_axi_card_hbm_p15_arvalid  ,
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p15_awaddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p15_awburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p15_awcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p15_awid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p15_awlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p15_awlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p15_awprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p15_awqos    ,
   input                                   m_axi_card_hbm_p15_awready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p15_awregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p15_awsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p15_awuser  ,
   output                                  m_axi_card_hbm_p15_awvalid  ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p15_bid      ,
   output                                  m_axi_card_hbm_p15_bready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p15_bresp    ,
   input [`AXI_CARD_HBM_USER_WIDTH-1 : 0]  m_axi_card_hbm_p15_buser    ,
   input                                   m_axi_card_hbm_p15_bvalid   ,
   input [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]   m_axi_card_hbm_p15_rdata    ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p15_rid      ,
   input                                   m_axi_card_hbm_p15_rlast    ,
   output                                  m_axi_card_hbm_p15_rready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p15_rresp    ,
   input [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p15_ruser    ,
   input                                   m_axi_card_hbm_p15_rvalid   ,
   output [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]  m_axi_card_hbm_p15_wdata    ,
   output                                  m_axi_card_hbm_p15_wlast    ,
   input                                   m_axi_card_hbm_p15_wready   ,
   output [(`AXI_CARD_HBM_DATA_WIDTH/8)-1 : 0] m_axi_card_hbm_p15_wstrb  ,
   output [`AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p15_wuser    ,
   output                                  m_axi_card_hbm_p15_wvalid   ,
   `endif

   `ifdef HBM_AXI_IF_P16
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p16_araddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p16_arburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p16_arcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p16_arid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p16_arlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p16_arlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p16_arprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p16_arqos    ,
   input                                   m_axi_card_hbm_p16_arready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p16_arregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p16_arsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p16_aruser  ,
   output                                  m_axi_card_hbm_p16_arvalid  ,
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p16_awaddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p16_awburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p16_awcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p16_awid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p16_awlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p16_awlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p16_awprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p16_awqos    ,
   input                                   m_axi_card_hbm_p16_awready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p16_awregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p16_awsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p16_awuser  ,
   output                                  m_axi_card_hbm_p16_awvalid  ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p16_bid      ,
   output                                  m_axi_card_hbm_p16_bready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p16_bresp    ,
   input [`AXI_CARD_HBM_USER_WIDTH-1 : 0]  m_axi_card_hbm_p16_buser    ,
   input                                   m_axi_card_hbm_p16_bvalid   ,
   input [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]   m_axi_card_hbm_p16_rdata    ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p16_rid      ,
   input                                   m_axi_card_hbm_p16_rlast    ,
   output                                  m_axi_card_hbm_p16_rready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p16_rresp    ,
   input [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p16_ruser    ,
   input                                   m_axi_card_hbm_p16_rvalid   ,
   output [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]  m_axi_card_hbm_p16_wdata    ,
   output                                  m_axi_card_hbm_p16_wlast    ,
   input                                   m_axi_card_hbm_p16_wready   ,
   output [(`AXI_CARD_HBM_DATA_WIDTH/8)-1 : 0] m_axi_card_hbm_p16_wstrb  ,
   output [`AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p16_wuser    ,
   output                                  m_axi_card_hbm_p16_wvalid   ,
   `endif

   `ifdef HBM_AXI_IF_P17
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p17_araddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p17_arburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p17_arcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p17_arid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p17_arlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p17_arlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p17_arprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p17_arqos    ,
   input                                   m_axi_card_hbm_p17_arready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p17_arregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p17_arsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p17_aruser  ,
   output                                  m_axi_card_hbm_p17_arvalid  ,
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p17_awaddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p17_awburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p17_awcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p17_awid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p17_awlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p17_awlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p17_awprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p17_awqos    ,
   input                                   m_axi_card_hbm_p17_awready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p17_awregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p17_awsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p17_awuser  ,
   output                                  m_axi_card_hbm_p17_awvalid  ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p17_bid      ,
   output                                  m_axi_card_hbm_p17_bready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p17_bresp    ,
   input [`AXI_CARD_HBM_USER_WIDTH-1 : 0]  m_axi_card_hbm_p17_buser    ,
   input                                   m_axi_card_hbm_p17_bvalid   ,
   input [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]   m_axi_card_hbm_p17_rdata    ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p17_rid      ,
   input                                   m_axi_card_hbm_p17_rlast    ,
   output                                  m_axi_card_hbm_p17_rready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p17_rresp    ,
   input [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p17_ruser    ,
   input                                   m_axi_card_hbm_p17_rvalid   ,
   output [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]  m_axi_card_hbm_p17_wdata    ,
   output                                  m_axi_card_hbm_p17_wlast    ,
   input                                   m_axi_card_hbm_p17_wready   ,
   output [(`AXI_CARD_HBM_DATA_WIDTH/8)-1 : 0] m_axi_card_hbm_p17_wstrb  ,
   output [`AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p17_wuser    ,
   output                                  m_axi_card_hbm_p17_wvalid   ,
   `endif

   `ifdef HBM_AXI_IF_P18
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p18_araddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p18_arburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p18_arcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p18_arid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p18_arlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p18_arlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p18_arprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p18_arqos    ,
   input                                   m_axi_card_hbm_p18_arready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p18_arregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p18_arsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p18_aruser  ,
   output                                  m_axi_card_hbm_p18_arvalid  ,
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p18_awaddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p18_awburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p18_awcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p18_awid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p18_awlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p18_awlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p18_awprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p18_awqos    ,
   input                                   m_axi_card_hbm_p18_awready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p18_awregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p18_awsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p18_awuser  ,
   output                                  m_axi_card_hbm_p18_awvalid  ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p18_bid      ,
   output                                  m_axi_card_hbm_p18_bready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p18_bresp    ,
   input [`AXI_CARD_HBM_USER_WIDTH-1 : 0]  m_axi_card_hbm_p18_buser    ,
   input                                   m_axi_card_hbm_p18_bvalid   ,
   input [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]   m_axi_card_hbm_p18_rdata    ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p18_rid      ,
   input                                   m_axi_card_hbm_p18_rlast    ,
   output                                  m_axi_card_hbm_p18_rready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p18_rresp    ,
   input [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p18_ruser    ,
   input                                   m_axi_card_hbm_p18_rvalid   ,
   output [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]  m_axi_card_hbm_p18_wdata    ,
   output                                  m_axi_card_hbm_p18_wlast    ,
   input                                   m_axi_card_hbm_p18_wready   ,
   output [(`AXI_CARD_HBM_DATA_WIDTH/8)-1 : 0] m_axi_card_hbm_p18_wstrb  ,
   output [`AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p18_wuser    ,
   output                                  m_axi_card_hbm_p18_wvalid   ,
   `endif

   `ifdef HBM_AXI_IF_P19
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p19_araddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p19_arburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p19_arcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p19_arid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p19_arlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p19_arlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p19_arprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p19_arqos    ,
   input                                   m_axi_card_hbm_p19_arready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p19_arregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p19_arsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p19_aruser  ,
   output                                  m_axi_card_hbm_p19_arvalid  ,
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p19_awaddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p19_awburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p19_awcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p19_awid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p19_awlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p19_awlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p19_awprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p19_awqos    ,
   input                                   m_axi_card_hbm_p19_awready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p19_awregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p19_awsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p19_awuser  ,
   output                                  m_axi_card_hbm_p19_awvalid  ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p19_bid      ,
   output                                  m_axi_card_hbm_p19_bready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p19_bresp    ,
   input [`AXI_CARD_HBM_USER_WIDTH-1 : 0]  m_axi_card_hbm_p19_buser    ,
   input                                   m_axi_card_hbm_p19_bvalid   ,
   input [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]   m_axi_card_hbm_p19_rdata    ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p19_rid      ,
   input                                   m_axi_card_hbm_p19_rlast    ,
   output                                  m_axi_card_hbm_p19_rready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p19_rresp    ,
   input [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p19_ruser    ,
   input                                   m_axi_card_hbm_p19_rvalid   ,
   output [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]  m_axi_card_hbm_p19_wdata    ,
   output                                  m_axi_card_hbm_p19_wlast    ,
   input                                   m_axi_card_hbm_p19_wready   ,
   output [(`AXI_CARD_HBM_DATA_WIDTH/8)-1 : 0] m_axi_card_hbm_p19_wstrb  ,
   output [`AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p19_wuser    ,
   output                                  m_axi_card_hbm_p19_wvalid   ,
   `endif

   `ifdef HBM_AXI_IF_P20
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p20_araddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p20_arburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p20_arcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p20_arid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p20_arlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p20_arlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p20_arprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p20_arqos    ,
   input                                   m_axi_card_hbm_p20_arready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p20_arregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p20_arsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p20_aruser  ,
   output                                  m_axi_card_hbm_p20_arvalid  ,
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p20_awaddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p20_awburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p20_awcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p20_awid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p20_awlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p20_awlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p20_awprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p20_awqos    ,
   input                                   m_axi_card_hbm_p20_awready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p20_awregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p20_awsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p20_awuser  ,
   output                                  m_axi_card_hbm_p20_awvalid  ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p20_bid      ,
   output                                  m_axi_card_hbm_p20_bready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p20_bresp    ,
   input [`AXI_CARD_HBM_USER_WIDTH-1 : 0]  m_axi_card_hbm_p20_buser    ,
   input                                   m_axi_card_hbm_p20_bvalid   ,
   input [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]   m_axi_card_hbm_p20_rdata    ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p20_rid      ,
   input                                   m_axi_card_hbm_p20_rlast    ,
   output                                  m_axi_card_hbm_p20_rready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p20_rresp    ,
   input [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p20_ruser    ,
   input                                   m_axi_card_hbm_p20_rvalid   ,
   output [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]  m_axi_card_hbm_p20_wdata    ,
   output                                  m_axi_card_hbm_p20_wlast    ,
   input                                   m_axi_card_hbm_p20_wready   ,
   output [(`AXI_CARD_HBM_DATA_WIDTH/8)-1 : 0] m_axi_card_hbm_p20_wstrb  ,
   output [`AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p20_wuser    ,
   output                                  m_axi_card_hbm_p20_wvalid   ,
   `endif

   `ifdef HBM_AXI_IF_P21
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p21_araddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p21_arburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p21_arcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p21_arid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p21_arlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p21_arlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p21_arprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p21_arqos    ,
   input                                   m_axi_card_hbm_p21_arready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p21_arregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p21_arsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p21_aruser  ,
   output                                  m_axi_card_hbm_p21_arvalid  ,
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p21_awaddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p21_awburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p21_awcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p21_awid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p21_awlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p21_awlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p21_awprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p21_awqos    ,
   input                                   m_axi_card_hbm_p21_awready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p21_awregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p21_awsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p21_awuser  ,
   output                                  m_axi_card_hbm_p21_awvalid  ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p21_bid      ,
   output                                  m_axi_card_hbm_p21_bready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p21_bresp    ,
   input [`AXI_CARD_HBM_USER_WIDTH-1 : 0]  m_axi_card_hbm_p21_buser    ,
   input                                   m_axi_card_hbm_p21_bvalid   ,
   input [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]   m_axi_card_hbm_p21_rdata    ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p21_rid      ,
   input                                   m_axi_card_hbm_p21_rlast    ,
   output                                  m_axi_card_hbm_p21_rready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p21_rresp    ,
   input [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p21_ruser    ,
   input                                   m_axi_card_hbm_p21_rvalid   ,
   output [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]  m_axi_card_hbm_p21_wdata    ,
   output                                  m_axi_card_hbm_p21_wlast    ,
   input                                   m_axi_card_hbm_p21_wready   ,
   output [(`AXI_CARD_HBM_DATA_WIDTH/8)-1 : 0] m_axi_card_hbm_p21_wstrb  ,
   output [`AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p21_wuser    ,
   output                                  m_axi_card_hbm_p21_wvalid   ,
   `endif

   `ifdef HBM_AXI_IF_P22
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p22_araddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p22_arburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p22_arcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p22_arid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p22_arlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p22_arlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p22_arprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p22_arqos    ,
   input                                   m_axi_card_hbm_p22_arready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p22_arregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p22_arsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p22_aruser  ,
   output                                  m_axi_card_hbm_p22_arvalid  ,
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p22_awaddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p22_awburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p22_awcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p22_awid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p22_awlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p22_awlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p22_awprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p22_awqos    ,
   input                                   m_axi_card_hbm_p22_awready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p22_awregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p22_awsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p22_awuser  ,
   output                                  m_axi_card_hbm_p22_awvalid  ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p22_bid      ,
   output                                  m_axi_card_hbm_p22_bready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p22_bresp    ,
   input [`AXI_CARD_HBM_USER_WIDTH-1 : 0]  m_axi_card_hbm_p22_buser    ,
   input                                   m_axi_card_hbm_p22_bvalid   ,
   input [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]   m_axi_card_hbm_p22_rdata    ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p22_rid      ,
   input                                   m_axi_card_hbm_p22_rlast    ,
   output                                  m_axi_card_hbm_p22_rready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p22_rresp    ,
   input [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p22_ruser    ,
   input                                   m_axi_card_hbm_p22_rvalid   ,
   output [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]  m_axi_card_hbm_p22_wdata    ,
   output                                  m_axi_card_hbm_p22_wlast    ,
   input                                   m_axi_card_hbm_p22_wready   ,
   output [(`AXI_CARD_HBM_DATA_WIDTH/8)-1 : 0] m_axi_card_hbm_p22_wstrb  ,
   output [`AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p22_wuser    ,
   output                                  m_axi_card_hbm_p22_wvalid   ,
   `endif

   `ifdef HBM_AXI_IF_P23
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p23_araddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p23_arburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p23_arcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p23_arid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p23_arlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p23_arlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p23_arprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p23_arqos    ,
   input                                   m_axi_card_hbm_p23_arready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p23_arregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p23_arsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p23_aruser  ,
   output                                  m_axi_card_hbm_p23_arvalid  ,
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p23_awaddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p23_awburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p23_awcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p23_awid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p23_awlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p23_awlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p23_awprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p23_awqos    ,
   input                                   m_axi_card_hbm_p23_awready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p23_awregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p23_awsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p23_awuser  ,
   output                                  m_axi_card_hbm_p23_awvalid  ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p23_bid      ,
   output                                  m_axi_card_hbm_p23_bready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p23_bresp    ,
   input [`AXI_CARD_HBM_USER_WIDTH-1 : 0]  m_axi_card_hbm_p23_buser    ,
   input                                   m_axi_card_hbm_p23_bvalid   ,
   input [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]   m_axi_card_hbm_p23_rdata    ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p23_rid      ,
   input                                   m_axi_card_hbm_p23_rlast    ,
   output                                  m_axi_card_hbm_p23_rready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p23_rresp    ,
   input [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p23_ruser    ,
   input                                   m_axi_card_hbm_p23_rvalid   ,
   output [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]  m_axi_card_hbm_p23_wdata    ,
   output                                  m_axi_card_hbm_p23_wlast    ,
   input                                   m_axi_card_hbm_p23_wready   ,
   output [(`AXI_CARD_HBM_DATA_WIDTH/8)-1 : 0] m_axi_card_hbm_p23_wstrb  ,
   output [`AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p23_wuser    ,
   output                                  m_axi_card_hbm_p23_wvalid   ,
   `endif

   `ifdef HBM_AXI_IF_P24
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p24_araddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p24_arburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p24_arcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p24_arid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p24_arlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p24_arlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p24_arprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p24_arqos    ,
   input                                   m_axi_card_hbm_p24_arready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p24_arregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p24_arsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p24_aruser  ,
   output                                  m_axi_card_hbm_p24_arvalid  ,
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p24_awaddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p24_awburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p24_awcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p24_awid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p24_awlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p24_awlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p24_awprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p24_awqos    ,
   input                                   m_axi_card_hbm_p24_awready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p24_awregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p24_awsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p24_awuser  ,
   output                                  m_axi_card_hbm_p24_awvalid  ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p24_bid      ,
   output                                  m_axi_card_hbm_p24_bready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p24_bresp    ,
   input [`AXI_CARD_HBM_USER_WIDTH-1 : 0]  m_axi_card_hbm_p24_buser    ,
   input                                   m_axi_card_hbm_p24_bvalid   ,
   input [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]   m_axi_card_hbm_p24_rdata    ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p24_rid      ,
   input                                   m_axi_card_hbm_p24_rlast    ,
   output                                  m_axi_card_hbm_p24_rready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p24_rresp    ,
   input [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p24_ruser    ,
   input                                   m_axi_card_hbm_p24_rvalid   ,
   output [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]  m_axi_card_hbm_p24_wdata    ,
   output                                  m_axi_card_hbm_p24_wlast    ,
   input                                   m_axi_card_hbm_p24_wready   ,
   output [(`AXI_CARD_HBM_DATA_WIDTH/8)-1 : 0] m_axi_card_hbm_p24_wstrb  ,
   output [`AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p24_wuser    ,
   output                                  m_axi_card_hbm_p24_wvalid   ,
   `endif

   `ifdef HBM_AXI_IF_P25
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p25_araddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p25_arburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p25_arcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p25_arid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p25_arlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p25_arlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p25_arprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p25_arqos    ,
   input                                   m_axi_card_hbm_p25_arready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p25_arregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p25_arsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p25_aruser  ,
   output                                  m_axi_card_hbm_p25_arvalid  ,
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p25_awaddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p25_awburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p25_awcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p25_awid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p25_awlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p25_awlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p25_awprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p25_awqos    ,
   input                                   m_axi_card_hbm_p25_awready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p25_awregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p25_awsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p25_awuser  ,
   output                                  m_axi_card_hbm_p25_awvalid  ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p25_bid      ,
   output                                  m_axi_card_hbm_p25_bready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p25_bresp    ,
   input [`AXI_CARD_HBM_USER_WIDTH-1 : 0]  m_axi_card_hbm_p25_buser    ,
   input                                   m_axi_card_hbm_p25_bvalid   ,
   input [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]   m_axi_card_hbm_p25_rdata    ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p25_rid      ,
   input                                   m_axi_card_hbm_p25_rlast    ,
   output                                  m_axi_card_hbm_p25_rready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p25_rresp    ,
   input [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p25_ruser    ,
   input                                   m_axi_card_hbm_p25_rvalid   ,
   output [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]  m_axi_card_hbm_p25_wdata    ,
   output                                  m_axi_card_hbm_p25_wlast    ,
   input                                   m_axi_card_hbm_p25_wready   ,
   output [(`AXI_CARD_HBM_DATA_WIDTH/8)-1 : 0] m_axi_card_hbm_p25_wstrb  ,
   output [`AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p25_wuser    ,
   output                                  m_axi_card_hbm_p25_wvalid   ,
   `endif

   `ifdef HBM_AXI_IF_P26
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p26_araddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p26_arburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p26_arcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p26_arid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p26_arlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p26_arlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p26_arprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p26_arqos    ,
   input                                   m_axi_card_hbm_p26_arready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p26_arregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p26_arsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p26_aruser  ,
   output                                  m_axi_card_hbm_p26_arvalid  ,
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p26_awaddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p26_awburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p26_awcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p26_awid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p26_awlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p26_awlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p26_awprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p26_awqos    ,
   input                                   m_axi_card_hbm_p26_awready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p26_awregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p26_awsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p26_awuser  ,
   output                                  m_axi_card_hbm_p26_awvalid  ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p26_bid      ,
   output                                  m_axi_card_hbm_p26_bready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p26_bresp    ,
   input [`AXI_CARD_HBM_USER_WIDTH-1 : 0]  m_axi_card_hbm_p26_buser    ,
   input                                   m_axi_card_hbm_p26_bvalid   ,
   input [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]   m_axi_card_hbm_p26_rdata    ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p26_rid      ,
   input                                   m_axi_card_hbm_p26_rlast    ,
   output                                  m_axi_card_hbm_p26_rready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p26_rresp    ,
   input [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p26_ruser    ,
   input                                   m_axi_card_hbm_p26_rvalid   ,
   output [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]  m_axi_card_hbm_p26_wdata    ,
   output                                  m_axi_card_hbm_p26_wlast    ,
   input                                   m_axi_card_hbm_p26_wready   ,
   output [(`AXI_CARD_HBM_DATA_WIDTH/8)-1 : 0] m_axi_card_hbm_p26_wstrb  ,
   output [`AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p26_wuser    ,
   output                                  m_axi_card_hbm_p26_wvalid   ,
   `endif

   `ifdef HBM_AXI_IF_P27
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p27_araddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p27_arburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p27_arcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p27_arid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p27_arlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p27_arlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p27_arprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p27_arqos    ,
   input                                   m_axi_card_hbm_p27_arready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p27_arregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p27_arsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p27_aruser  ,
   output                                  m_axi_card_hbm_p27_arvalid  ,
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p27_awaddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p27_awburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p27_awcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p27_awid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p27_awlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p27_awlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p27_awprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p27_awqos    ,
   input                                   m_axi_card_hbm_p27_awready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p27_awregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p27_awsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p27_awuser  ,
   output                                  m_axi_card_hbm_p27_awvalid  ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p27_bid      ,
   output                                  m_axi_card_hbm_p27_bready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p27_bresp    ,
   input [`AXI_CARD_HBM_USER_WIDTH-1 : 0]  m_axi_card_hbm_p27_buser    ,
   input                                   m_axi_card_hbm_p27_bvalid   ,
   input [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]   m_axi_card_hbm_p27_rdata    ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p27_rid      ,
   input                                   m_axi_card_hbm_p27_rlast    ,
   output                                  m_axi_card_hbm_p27_rready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p27_rresp    ,
   input [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p27_ruser    ,
   input                                   m_axi_card_hbm_p27_rvalid   ,
   output [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]  m_axi_card_hbm_p27_wdata    ,
   output                                  m_axi_card_hbm_p27_wlast    ,
   input                                   m_axi_card_hbm_p27_wready   ,
   output [(`AXI_CARD_HBM_DATA_WIDTH/8)-1 : 0] m_axi_card_hbm_p27_wstrb  ,
   output [`AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p27_wuser    ,
   output                                  m_axi_card_hbm_p27_wvalid   ,
   `endif

   `ifdef HBM_AXI_IF_P28
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p28_araddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p28_arburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p28_arcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p28_arid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p28_arlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p28_arlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p28_arprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p28_arqos    ,
   input                                   m_axi_card_hbm_p28_arready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p28_arregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p28_arsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p28_aruser  ,
   output                                  m_axi_card_hbm_p28_arvalid  ,
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p28_awaddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p28_awburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p28_awcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p28_awid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p28_awlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p28_awlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p28_awprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p28_awqos    ,
   input                                   m_axi_card_hbm_p28_awready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p28_awregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p28_awsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p28_awuser  ,
   output                                  m_axi_card_hbm_p28_awvalid  ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p28_bid      ,
   output                                  m_axi_card_hbm_p28_bready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p28_bresp    ,
   input [`AXI_CARD_HBM_USER_WIDTH-1 : 0]  m_axi_card_hbm_p28_buser    ,
   input                                   m_axi_card_hbm_p28_bvalid   ,
   input [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]   m_axi_card_hbm_p28_rdata    ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p28_rid      ,
   input                                   m_axi_card_hbm_p28_rlast    ,
   output                                  m_axi_card_hbm_p28_rready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p28_rresp    ,
   input [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p28_ruser    ,
   input                                   m_axi_card_hbm_p28_rvalid   ,
   output [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]  m_axi_card_hbm_p28_wdata    ,
   output                                  m_axi_card_hbm_p28_wlast    ,
   input                                   m_axi_card_hbm_p28_wready   ,
   output [(`AXI_CARD_HBM_DATA_WIDTH/8)-1 : 0] m_axi_card_hbm_p28_wstrb  ,
   output [`AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p28_wuser    ,
   output                                  m_axi_card_hbm_p28_wvalid   ,
   `endif

   `ifdef HBM_AXI_IF_P29
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p29_araddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p29_arburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p29_arcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p29_arid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p29_arlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p29_arlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p29_arprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p29_arqos    ,
   input                                   m_axi_card_hbm_p29_arready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p29_arregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p29_arsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p29_aruser  ,
   output                                  m_axi_card_hbm_p29_arvalid  ,
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p29_awaddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p29_awburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p29_awcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p29_awid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p29_awlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p29_awlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p29_awprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p29_awqos    ,
   input                                   m_axi_card_hbm_p29_awready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p29_awregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p29_awsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p29_awuser  ,
   output                                  m_axi_card_hbm_p29_awvalid  ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p29_bid      ,
   output                                  m_axi_card_hbm_p29_bready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p29_bresp    ,
   input [`AXI_CARD_HBM_USER_WIDTH-1 : 0]  m_axi_card_hbm_p29_buser    ,
   input                                   m_axi_card_hbm_p29_bvalid   ,
   input [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]   m_axi_card_hbm_p29_rdata    ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p29_rid      ,
   input                                   m_axi_card_hbm_p29_rlast    ,
   output                                  m_axi_card_hbm_p29_rready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p29_rresp    ,
   input [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p29_ruser    ,
   input                                   m_axi_card_hbm_p29_rvalid   ,
   output [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]  m_axi_card_hbm_p29_wdata    ,
   output                                  m_axi_card_hbm_p29_wlast    ,
   input                                   m_axi_card_hbm_p29_wready   ,
   output [(`AXI_CARD_HBM_DATA_WIDTH/8)-1 : 0] m_axi_card_hbm_p29_wstrb  ,
   output [`AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p29_wuser    ,
   output                                  m_axi_card_hbm_p29_wvalid   ,
   `endif

   `ifdef HBM_AXI_IF_P30
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p30_araddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p30_arburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p30_arcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p30_arid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p30_arlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p30_arlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p30_arprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p30_arqos    ,
   input                                   m_axi_card_hbm_p30_arready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p30_arregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p30_arsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p30_aruser  ,
   output                                  m_axi_card_hbm_p30_arvalid  ,
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p30_awaddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p30_awburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p30_awcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p30_awid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p30_awlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p30_awlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p30_awprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p30_awqos    ,
   input                                   m_axi_card_hbm_p30_awready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p30_awregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p30_awsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p30_awuser  ,
   output                                  m_axi_card_hbm_p30_awvalid  ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p30_bid      ,
   output                                  m_axi_card_hbm_p30_bready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p30_bresp    ,
   input [`AXI_CARD_HBM_USER_WIDTH-1 : 0]  m_axi_card_hbm_p30_buser    ,
   input                                   m_axi_card_hbm_p30_bvalid   ,
   input [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]   m_axi_card_hbm_p30_rdata    ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p30_rid      ,
   input                                   m_axi_card_hbm_p30_rlast    ,
   output                                  m_axi_card_hbm_p30_rready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p30_rresp    ,
   input [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p30_ruser    ,
   input                                   m_axi_card_hbm_p30_rvalid   ,
   output [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]  m_axi_card_hbm_p30_wdata    ,
   output                                  m_axi_card_hbm_p30_wlast    ,
   input                                   m_axi_card_hbm_p30_wready   ,
   output [(`AXI_CARD_HBM_DATA_WIDTH/8)-1 : 0] m_axi_card_hbm_p30_wstrb  ,
   output [`AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p30_wuser    ,
   output                                  m_axi_card_hbm_p30_wvalid   ,
   `endif

   `ifdef HBM_AXI_IF_P31
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p31_araddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p31_arburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p31_arcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p31_arid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p31_arlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p31_arlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p31_arprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p31_arqos    ,
   input                                   m_axi_card_hbm_p31_arready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p31_arregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p31_arsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p31_aruser  ,
   output                                  m_axi_card_hbm_p31_arvalid  ,
   output [ `AXI_CARD_HBM_ADDR_WIDTH-1 : 0]  m_axi_card_hbm_p31_awaddr   ,
   output [ 1 : 0]                         m_axi_card_hbm_p31_awburst  ,
   output [ 3 : 0]                         m_axi_card_hbm_p31_awcache  ,
   output [ `AXI_CARD_HBM_ID_WIDTH-1 : 0]    m_axi_card_hbm_p31_awid     ,
   output [ 7 : 0]                         m_axi_card_hbm_p31_awlen    ,
   output [ 1 : 0]                         m_axi_card_hbm_p31_awlock   ,
   output [ 2 : 0]                         m_axi_card_hbm_p31_awprot   ,
   output [ 3 : 0]                         m_axi_card_hbm_p31_awqos    ,
   input                                   m_axi_card_hbm_p31_awready  ,
   output [ 3 : 0]                         m_axi_card_hbm_p31_awregion ,
   output [ 2 : 0]                         m_axi_card_hbm_p31_awsize   ,
   output [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p31_awuser  ,
   output                                  m_axi_card_hbm_p31_awvalid  ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p31_bid      ,
   output                                  m_axi_card_hbm_p31_bready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p31_bresp    ,
   input [`AXI_CARD_HBM_USER_WIDTH-1 : 0]  m_axi_card_hbm_p31_buser    ,
   input                                   m_axi_card_hbm_p31_bvalid   ,
   input [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]   m_axi_card_hbm_p31_rdata    ,
   input [`AXI_CARD_HBM_ID_WIDTH-1 : 0]     m_axi_card_hbm_p31_rid      ,
   input                                   m_axi_card_hbm_p31_rlast    ,
   output                                  m_axi_card_hbm_p31_rready   ,
   input [ 1 : 0]                          m_axi_card_hbm_p31_rresp    ,
   input [ `AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p31_ruser    ,
   input                                   m_axi_card_hbm_p31_rvalid   ,
   output [`AXI_CARD_HBM_DATA_WIDTH-1 : 0]  m_axi_card_hbm_p31_wdata    ,
   output                                  m_axi_card_hbm_p31_wlast    ,
   input                                   m_axi_card_hbm_p31_wready   ,
   output [(`AXI_CARD_HBM_DATA_WIDTH/8)-1 : 0] m_axi_card_hbm_p31_wstrb  ,
   output [`AXI_CARD_HBM_USER_WIDTH-1 : 0] m_axi_card_hbm_p31_wuser    ,
   output                                  m_axi_card_hbm_p31_wvalid   ,
   `endif
`endif
`endif
    //
    // ETHERNET interface
`ifdef ENABLE_ETHERNET
`ifndef ENABLE_ETH_LOOP_BACK
// we define ethernet pins only if connected to an emac (no loopback)
   input [ 511 : 0]                        din_eth_tdata    ,
   input                                   din_eth_tvalid   ,
   output                                  din_eth_tready   ,
   input [  63 : 0]                        din_eth_tkeep    ,
   input [   0 : 0]                        din_eth_tuser    ,
   input                                   din_eth_tlast    ,
// Enable for ethernet TX
   output [ 511 : 0]                        dout_eth_tdata    ,
   output                                   dout_eth_tvalid   ,
   input                                    dout_eth_tready   ,
   output [  63 : 0]                        dout_eth_tkeep    ,
   output [   0 : 0]                        dout_eth_tuser    ,
   output                                   dout_eth_tlast    ,
   output                                   eth_rx_fifo_reset ,
   input                                    eth_stat_rx_status,
   input                                    eth_stat_rx_aligned,
`endif
`endif
    //
    // AXI Host Memory inputterface
   output [ `AXI_MM_AW-1 : 0]              m_axi_host_mem_araddr    ,
   output [ 1 : 0]                         m_axi_host_mem_arburst   ,
   output [ 3 : 0]                         m_axi_host_mem_arcache   ,
   output [ `IDW-1 : 0]                    m_axi_host_mem_arid      ,
   output [ 7 : 0]                         m_axi_host_mem_arlen     ,
   output [ 1 : 0]                         m_axi_host_mem_arlock    ,
   output [ 2 : 0]                         m_axi_host_mem_arprot    ,
   output [ 3 : 0]                         m_axi_host_mem_arqos     ,
   input                                   m_axi_host_mem_arready   ,
   output [ 3 : 0]                         m_axi_host_mem_arregion  ,
   output [ 2 : 0]                         m_axi_host_mem_arsize    ,
   output [ `AXI_ARUSER-1 : 0]             m_axi_host_mem_aruser    ,
   output                                  m_axi_host_mem_arvalid   ,
   output [ `AXI_MM_AW-1 : 0]              m_axi_host_mem_awaddr    ,
   output [ 1 : 0]                         m_axi_host_mem_awburst   ,
   output [ 3 : 0]                         m_axi_host_mem_awcache   ,
   output [ `IDW-1 : 0]                    m_axi_host_mem_awid      ,
   output [ 7 : 0]                         m_axi_host_mem_awlen     ,
   output [ 1 : 0]                         m_axi_host_mem_awlock    ,
   output [ 2 : 0]                         m_axi_host_mem_awprot    ,
   output [ 3 : 0]                         m_axi_host_mem_awqos     ,
   input                                   m_axi_host_mem_awready   ,
   output [ 3 : 0]                         m_axi_host_mem_awregion  ,
   output [ 2 : 0]                         m_axi_host_mem_awsize    ,
   output [`AXI_AWUSER-1 : 0]              m_axi_host_mem_awuser    ,
   output                                  m_axi_host_mem_awvalid   ,
   input [ `IDW-1 : 0]                     m_axi_host_mem_bid       ,
   output                                  m_axi_host_mem_bready    ,
   input [ 1 : 0]                          m_axi_host_mem_bresp     ,
   input [ `AXI_BUSER-1 : 0]               m_axi_host_mem_buser     ,
   input                                   m_axi_host_mem_bvalid    ,
   input [ `AXI_ACT_DW-1 : 0]              m_axi_host_mem_rdata     ,
   input [ `IDW-1 : 0]                     m_axi_host_mem_rid       ,
   input                                   m_axi_host_mem_rlast     ,
   output                                  m_axi_host_mem_rready    ,
   input [ 1 : 0]                          m_axi_host_mem_rresp     ,
   input [ `AXI_RUSER-1 : 0]               m_axi_host_mem_ruser     ,
   input                                   m_axi_host_mem_rvalid    ,
   output [ `AXI_ACT_DW-1 : 0]             m_axi_host_mem_wdata     ,
   output                                  m_axi_host_mem_wlast     ,
   input                                   m_axi_host_mem_wready    ,
   output [(`AXI_ACT_DW/8)-1 : 0]          m_axi_host_mem_wstrb     ,
   output [ `AXI_WUSER-1 : 0]              m_axi_host_mem_wuser     ,
   output                                  m_axi_host_mem_wvalid
    );


parameter ADDR_ACTION_TYPE = 32'h10;
parameter ADDR_RELEASE_LEVEL = 32'h14;
parameter ADDR_ACTION_INTERRUPT_SRC_ADDR_LO = 32'h18;
parameter ADDR_ACTION_INTERRUPT_SRC_ADDR_HI = 32'h1C;

reg context_q;
reg [31:0] interrupt_src_hi;
reg [31:0] interrupt_src_lo;
reg interrupt_q;
reg interrupt_wait_ack_q;
reg hls_rst_n_q;
wire interrupt_i;
wire [63:0] temp_card_mem0_araddr;
wire [63:0] temp_card_mem0_awaddr;
   `ifdef HBM_AXI_IF_P0
wire [63:0] temp_card_hbm_p0_araddr;
wire [63:0] temp_card_hbm_p0_awaddr;
   `endif
   `ifdef HBM_AXI_IF_P1
wire [63:0] temp_card_hbm_p1_araddr;
wire [63:0] temp_card_hbm_p1_awaddr;
   `endif
   `ifdef HBM_AXI_IF_P2
wire [63:0] temp_card_hbm_p2_araddr;
wire [63:0] temp_card_hbm_p2_awaddr;
   `endif
   `ifdef HBM_AXI_IF_P3
wire [63:0] temp_card_hbm_p3_araddr;
wire [63:0] temp_card_hbm_p3_awaddr;
   `endif
   `ifdef HBM_AXI_IF_P4
wire [63:0] temp_card_hbm_p4_araddr;
wire [63:0] temp_card_hbm_p4_awaddr;
   `endif
   `ifdef HBM_AXI_IF_P5
wire [63:0] temp_card_hbm_p5_araddr;
wire [63:0] temp_card_hbm_p5_awaddr;
   `endif
   `ifdef HBM_AXI_IF_P6
wire [63:0] temp_card_hbm_p6_araddr;
wire [63:0] temp_card_hbm_p6_awaddr;
   `endif
   `ifdef HBM_AXI_IF_P7
wire [63:0] temp_card_hbm_p7_araddr;
wire [63:0] temp_card_hbm_p7_awaddr;
   `endif
   `ifdef HBM_AXI_IF_P8
wire [63:0] temp_card_hbm_p8_araddr;
wire [63:0] temp_card_hbm_p8_awaddr;
   `endif
   `ifdef HBM_AXI_IF_P9
wire [63:0] temp_card_hbm_p9_araddr;
wire [63:0] temp_card_hbm_p9_awaddr;
   `endif
   `ifdef HBM_AXI_IF_P10
wire [63:0] temp_card_hbm_p10_araddr;
wire [63:0] temp_card_hbm_p10_awaddr;
   `endif
   `ifdef HBM_AXI_IF_P11
wire [63:0] temp_card_hbm_p11_araddr;
wire [63:0] temp_card_hbm_p11_awaddr;
   `endif
   `ifdef HBM_AXI_IF_P12
wire [63:0] temp_card_hbm_p12_araddr;
wire [63:0] temp_card_hbm_p12_awaddr;
   `endif
   `ifdef HBM_AXI_IF_P13
wire [63:0] temp_card_hbm_p13_araddr;
wire [63:0] temp_card_hbm_p13_awaddr;
   `endif
   `ifdef HBM_AXI_IF_P14
wire [63:0] temp_card_hbm_p14_araddr;
wire [63:0] temp_card_hbm_p14_awaddr;
   `endif
   `ifdef HBM_AXI_IF_P15
wire [63:0] temp_card_hbm_p15_araddr;
wire [63:0] temp_card_hbm_p15_awaddr;
   `endif
   `ifdef HBM_AXI_IF_P16
wire [63:0] temp_card_hbm_p16_araddr;
wire [63:0] temp_card_hbm_p16_awaddr;
   `endif
   `ifdef HBM_AXI_IF_P17
wire [63:0] temp_card_hbm_p17_araddr;
wire [63:0] temp_card_hbm_p17_awaddr;
   `endif
   `ifdef HBM_AXI_IF_P18
wire [63:0] temp_card_hbm_p18_araddr;
wire [63:0] temp_card_hbm_p18_awaddr;
   `endif
   `ifdef HBM_AXI_IF_P19
wire [63:0] temp_card_hbm_p19_araddr;
wire [63:0] temp_card_hbm_p19_awaddr;
   `endif
   `ifdef HBM_AXI_IF_P20
wire [63:0] temp_card_hbm_p20_araddr;
wire [63:0] temp_card_hbm_p20_awaddr;
   `endif
   `ifdef HBM_AXI_IF_P21
wire [63:0] temp_card_hbm_p21_araddr;
wire [63:0] temp_card_hbm_p21_awaddr;
   `endif
   `ifdef HBM_AXI_IF_P22
wire [63:0] temp_card_hbm_p22_araddr;
wire [63:0] temp_card_hbm_p22_awaddr;
   `endif
   `ifdef HBM_AXI_IF_P23
wire [63:0] temp_card_hbm_p23_araddr;
wire [63:0] temp_card_hbm_p23_awaddr;
   `endif
   `ifdef HBM_AXI_IF_P24
wire [63:0] temp_card_hbm_p24_araddr;
wire [63:0] temp_card_hbm_p24_awaddr;
   `endif
   `ifdef HBM_AXI_IF_P25
wire [63:0] temp_card_hbm_p25_araddr;
wire [63:0] temp_card_hbm_p25_awaddr;
   `endif
   `ifdef HBM_AXI_IF_P26
wire [63:0] temp_card_hbm_p26_araddr;
wire [63:0] temp_card_hbm_p26_awaddr;
   `endif
   `ifdef HBM_AXI_IF_P27
wire [63:0] temp_card_hbm_p27_araddr;
wire [63:0] temp_card_hbm_p27_awaddr;
   `endif
   `ifdef HBM_AXI_IF_P28
wire [63:0] temp_card_hbm_p28_araddr;
wire [63:0] temp_card_hbm_p28_awaddr;
   `endif
   `ifdef HBM_AXI_IF_P29
wire [63:0] temp_card_hbm_p29_araddr;
wire [63:0] temp_card_hbm_p29_awaddr;
   `endif
   `ifdef HBM_AXI_IF_P30
wire [63:0] temp_card_hbm_p30_araddr;
wire [63:0] temp_card_hbm_p30_awaddr;
   `endif
   `ifdef HBM_AXI_IF_P31
wire [63:0] temp_card_hbm_p31_araddr;
wire [63:0] temp_card_hbm_p31_awaddr;
   `endif

    // ETHERNET interface
    // we define the ethernet wrap signals only if ethernet loop back
`ifdef ENABLE_ETHERNET
`ifdef ENABLE_ETH_LOOP_BACK
wire [511:0] dwrap_eth_TDATA;
wire dwrap_eth_TVALID;
wire dwrap_eth_TREADY;
wire [63:0] dwrap_eth_TKEEP;
wire [0:0] dwrap_eth_TUSER;
wire [0:0] dwrap_eth_TLAST;
`endif
`endif



reg  [31:0] reg_rdata_hijack; //This will be ORed with the return data of hls_action
wire [31:0] temp_s_axi_ctrl_reg_rdata;

  bd_action bd_action_metalfs (
    .ap_clk                       ( ap_clk                  ) ,
    .ap_rst_n                     ( hls_rst_n_q             ) ,
    .m_axi_card_mem0_araddr       (temp_card_mem0_araddr    ) ,
    .m_axi_card_mem0_arburst      (m_axi_card_mem0_arburst  ) ,
    .m_axi_card_mem0_arcache      (m_axi_card_mem0_arcache  ) ,
    .m_axi_card_mem0_arlen        (m_axi_card_mem0_arlen    ) ,
//    .m_axi_card_mem0_arlock       (m_axi_card_mem0_arlock   ) ,
    .m_axi_card_mem0_arprot       (m_axi_card_mem0_arprot   ) ,
    .m_axi_card_mem0_arqos        (m_axi_card_mem0_arqos    ) ,
    .m_axi_card_mem0_arready      (m_axi_card_mem0_arready  ) ,
    .m_axi_card_mem0_arregion     (m_axi_card_mem0_arregion ) ,
    .m_axi_card_mem0_arsize       (m_axi_card_mem0_arsize   ) ,
    .m_axi_card_mem0_aruser       (m_axi_card_mem0_aruser   ) ,
    .m_axi_card_mem0_arvalid      (m_axi_card_mem0_arvalid  ) ,
    .m_axi_card_mem0_awaddr       (temp_card_mem0_awaddr    ) ,
    .m_axi_card_mem0_awburst      (m_axi_card_mem0_awburst  ) ,
    .m_axi_card_mem0_awcache      (m_axi_card_mem0_awcache  ) ,
    .m_axi_card_mem0_awlen        (m_axi_card_mem0_awlen    ) ,
//    .m_axi_card_mem0_awlock       (m_axi_card_mem0_awlock   ) ,
    .m_axi_card_mem0_awprot       (m_axi_card_mem0_awprot   ) ,
    .m_axi_card_mem0_awqos        (m_axi_card_mem0_awqos    ) ,
    .m_axi_card_mem0_awready      (m_axi_card_mem0_awready  ) ,
    .m_axi_card_mem0_awregion     (m_axi_card_mem0_awregion ) ,
    .m_axi_card_mem0_awsize       (m_axi_card_mem0_awsize   ) ,
    .m_axi_card_mem0_awuser       (m_axi_card_mem0_awuser   ) ,
    .m_axi_card_mem0_awvalid      (m_axi_card_mem0_awvalid  ) ,
    .m_axi_card_mem0_bready       (m_axi_card_mem0_bready   ) ,
    .m_axi_card_mem0_bresp        (m_axi_card_mem0_bresp    ) ,
    .m_axi_card_mem0_buser        (m_axi_card_mem0_buser    ) ,
    .m_axi_card_mem0_bvalid       (m_axi_card_mem0_bvalid   ) ,
    .m_axi_card_mem0_rdata        (m_axi_card_mem0_rdata    ) ,
    .m_axi_card_mem0_rlast        (m_axi_card_mem0_rlast    ) ,
    .m_axi_card_mem0_rready       (m_axi_card_mem0_rready   ) ,
    .m_axi_card_mem0_rresp        (m_axi_card_mem0_rresp    ) ,
    .m_axi_card_mem0_ruser        (m_axi_card_mem0_ruser    ) ,
    .m_axi_card_mem0_rvalid       (m_axi_card_mem0_rvalid   ) ,
    .m_axi_card_mem0_wdata        (m_axi_card_mem0_wdata    ) ,
    .m_axi_card_mem0_wlast        (m_axi_card_mem0_wlast    ) ,
    .m_axi_card_mem0_wready       (m_axi_card_mem0_wready   ) ,
    .m_axi_card_mem0_wstrb        (m_axi_card_mem0_wstrb    ) ,
    .m_axi_card_mem0_wuser        (m_axi_card_mem0_wuser    ) ,
    .m_axi_card_mem0_wvalid       (m_axi_card_mem0_wvalid   ) ,

    .s_axi_ctrl_reg_araddr        (s_axi_ctrl_reg_araddr    ) ,
    .s_axi_ctrl_reg_arready       (s_axi_ctrl_reg_arready   ) ,
    .s_axi_ctrl_reg_arvalid       (s_axi_ctrl_reg_arvalid   ) ,
    .s_axi_ctrl_reg_awaddr        (s_axi_ctrl_reg_awaddr    ) ,
    .s_axi_ctrl_reg_awready       (s_axi_ctrl_reg_awready   ) ,
    .s_axi_ctrl_reg_awvalid       (s_axi_ctrl_reg_awvalid   ) ,
    .s_axi_ctrl_reg_bready        (s_axi_ctrl_reg_bready    ) ,
    .s_axi_ctrl_reg_bresp         (s_axi_ctrl_reg_bresp     ) ,
    .s_axi_ctrl_reg_bvalid        (s_axi_ctrl_reg_bvalid    ) ,
    .s_axi_ctrl_reg_rdata         (temp_s_axi_ctrl_reg_rdata     ) ,
    .s_axi_ctrl_reg_rready        (s_axi_ctrl_reg_rready    ) ,
    .s_axi_ctrl_reg_rresp         (s_axi_ctrl_reg_rresp     ) ,
    .s_axi_ctrl_reg_rvalid        (s_axi_ctrl_reg_rvalid    ) ,
    .s_axi_ctrl_reg_wdata         (s_axi_ctrl_reg_wdata     ) ,
    .s_axi_ctrl_reg_wready        (s_axi_ctrl_reg_wready    ) ,
    .s_axi_ctrl_reg_wstrb         (s_axi_ctrl_reg_wstrb     ) ,
    .s_axi_ctrl_reg_wvalid        (s_axi_ctrl_reg_wvalid    ) ,

    .m_axi_host_mem_araddr        (m_axi_host_mem_araddr    ) ,
    .m_axi_host_mem_arburst       (m_axi_host_mem_arburst   ) ,
    .m_axi_host_mem_arcache       (m_axi_host_mem_arcache   ) ,
    .m_axi_host_mem_arid          (m_axi_host_mem_arid [0]  ) ,//SR# 10394170
    .m_axi_host_mem_arlen         (m_axi_host_mem_arlen     ) ,
//    .m_axi_host_mem_arlock        (m_axi_host_mem_arlock    ) ,
    .m_axi_host_mem_arprot        (m_axi_host_mem_arprot    ) ,
    .m_axi_host_mem_arqos         (m_axi_host_mem_arqos     ) ,
    .m_axi_host_mem_arready       (m_axi_host_mem_arready   ) ,
    .m_axi_host_mem_arregion      (m_axi_host_mem_arregion  ) ,
    .m_axi_host_mem_arsize        (m_axi_host_mem_arsize    ) ,
    .m_axi_host_mem_aruser        (                         ) ,
    .m_axi_host_mem_arvalid       (m_axi_host_mem_arvalid   ) ,
    .m_axi_host_mem_awaddr        (m_axi_host_mem_awaddr    ) ,
    .m_axi_host_mem_awburst       (m_axi_host_mem_awburst   ) ,
    .m_axi_host_mem_awcache       (m_axi_host_mem_awcache   ) ,
    .m_axi_host_mem_awid          (m_axi_host_mem_awid [0]  ) ,//SR# 10394170
    .m_axi_host_mem_awlen         (m_axi_host_mem_awlen     ) ,
//    .m_axi_host_mem_awlock        (m_axi_host_mem_awlock    ) ,
    .m_axi_host_mem_awprot        (m_axi_host_mem_awprot    ) ,
    .m_axi_host_mem_awqos         (m_axi_host_mem_awqos     ) ,
    .m_axi_host_mem_awready       (m_axi_host_mem_awready   ) ,
    .m_axi_host_mem_awregion      (m_axi_host_mem_awregion  ) ,
    .m_axi_host_mem_awsize        (m_axi_host_mem_awsize    ) ,
    .m_axi_host_mem_awuser        (                         ) ,
    .m_axi_host_mem_awvalid       (m_axi_host_mem_awvalid   ) ,
    .m_axi_host_mem_bid           (m_axi_host_mem_bid [0]   ) ,//SR# 10394170
    .m_axi_host_mem_bready        (m_axi_host_mem_bready    ) ,
    .m_axi_host_mem_bresp         (m_axi_host_mem_bresp     ) ,
    .m_axi_host_mem_buser         (m_axi_host_mem_buser [0] ) ,//SR# 10394170
    .m_axi_host_mem_bvalid        (m_axi_host_mem_bvalid    ) ,
    .m_axi_host_mem_rdata         (m_axi_host_mem_rdata     ) ,
    .m_axi_host_mem_rid           (m_axi_host_mem_rid [0]   ) ,//SR# 10394170
    .m_axi_host_mem_rlast         (m_axi_host_mem_rlast     ) ,
    .m_axi_host_mem_rready        (m_axi_host_mem_rready    ) ,
    .m_axi_host_mem_rresp         (m_axi_host_mem_rresp     ) ,
    .m_axi_host_mem_ruser         (m_axi_host_mem_ruser [0] ) ,//SR# 10394170
    .m_axi_host_mem_rvalid        (m_axi_host_mem_rvalid    ) ,
    .m_axi_host_mem_wdata         (m_axi_host_mem_wdata     ) ,
//    .m_axi_host_mem_wid           (                         ) ,
    .m_axi_host_mem_wlast         (m_axi_host_mem_wlast     ) ,
    .m_axi_host_mem_wready        (m_axi_host_mem_wready    ) ,
    .m_axi_host_mem_wstrb         (m_axi_host_mem_wstrb     ) ,
    .m_axi_host_mem_wuser         (m_axi_host_mem_wuser [0] ) ,//SR# 10394170
    .m_axi_host_mem_wvalid        (m_axi_host_mem_wvalid    ) ,
    .interrupt                    ( interrupt_i             )
  );

// bd_action does not use card_mem0 ids
  assign  m_axi_card_mem0_arid  [ 0 ] = 'b0;
  assign  m_axi_card_mem0_awid  [ 0 ] = 'b0;

// bd_action does not use lock signals
  assign m_axi_host_mem_awlock = 'b0;
  assign m_axi_host_mem_arlock = 'b0;
  assign m_axi_card_mem0_awlock = 'b0;
  assign m_axi_card_mem0_arlock = 'b0;

//==========================================
// Reset for hls_action
always @ (posedge ap_clk)
     hls_rst_n_q <= ap_rst_n;

//==========================================
// Context is not implemented
always @ (posedge ap_clk)
    if (~ap_rst_n)
        context_q <= 0;
//    else if (s_axi_ctrl_reg_wvalid && (s_axi_ctrl_reg_awaddr = ADDR_CTX_ID_REG )
//        context_q <= s_axi_ctrl_reg_wdata;


//==========================================
// Interrupt handshaking logic
always @ (posedge ap_clk)
     if (~ap_rst_n) begin
        interrupt_q          <= 1'b0;
        interrupt_wait_ack_q <= 1'b0;
     end
     else begin
         interrupt_wait_ack_q <= (interrupt_i & ~interrupt_q ) | (interrupt_wait_ack_q & ~interrupt_ack);
         interrupt_q          <= interrupt_i & (interrupt_q | ~interrupt_wait_ack_q);
     end

// Interrupt output signals
  // Generating interrupt pulse
assign  interrupt     = interrupt_i & ~interrupt_q;
  // use fixed interrupt source id '0x4' for HLS interrupts
  // (the high order bit of the source id is assigned by SNAP)
always @ (posedge ap_clk)
    if (~ap_rst_n) begin
        interrupt_src_hi <= 32'b0;
        interrupt_src_lo <= 32'b0;
    end
    else if (s_axi_ctrl_reg_wvalid  && (s_axi_ctrl_reg_awaddr == ADDR_ACTION_INTERRUPT_SRC_ADDR_HI))
        interrupt_src_hi <= s_axi_ctrl_reg_wdata;
    else if (s_axi_ctrl_reg_wvalid  && (s_axi_ctrl_reg_awaddr == ADDR_ACTION_INTERRUPT_SRC_ADDR_LO))
        interrupt_src_lo <= s_axi_ctrl_reg_wdata;

assign  interrupt_src = {interrupt_src_hi, interrupt_src_lo};
  // context ID
assign  interrupt_ctx = context_q;


//==========================================
//When read ACTION_TYPE and RELEASE_LEVEL, the return data is handled here. 
//hls_action will return RVALID (acknowledgement), RDATA=0
//and RDATA is ORed with this reg_rdata_hijack. 
always @ (posedge ap_clk)
    if (~ap_rst_n) begin
        reg_rdata_hijack <= 32'h0;
    end
    else if (s_axi_ctrl_reg_arvalid == 1'b1) begin
        if (s_axi_ctrl_reg_araddr == ADDR_ACTION_TYPE)
            reg_rdata_hijack <= 32'hFB060001;
        else if (s_axi_ctrl_reg_araddr == ADDR_RELEASE_LEVEL)
            reg_rdata_hijack <= 32'h00000001;
        else
            reg_rdata_hijack <= 32'h0;
    end

assign s_axi_ctrl_reg_rdata = reg_rdata_hijack | temp_s_axi_ctrl_reg_rdata;

//==========================================
// Driving context ID to host memory interface
assign  m_axi_host_mem_aruser = context_q;
assign  m_axi_host_mem_awuser = context_q;

// Driving the higher ID fields to 0.
generate if(`IDW > 1)
begin:high_hid_fields_driver
    assign  m_axi_host_mem_arid  [ `IDW-1 : 1 ] = 'b0;
    assign  m_axi_host_mem_awid  [ `IDW-1 : 1 ] = 'b0;
end
endgenerate
//assign  m_axi_host_mem_wuser [ `AXI_WUSER-1 : 1 ] = 'b0;


// if DDR or DDR replaced by BRAM
`ifdef ENABLE_AXI_CARD_MEM
`ifndef ENABLE_HBM
assign m_axi_card_mem0_araddr = temp_card_mem0_araddr[`AXI_CARD_MEM_ADDR_WIDTH-1:0];
assign m_axi_card_mem0_awaddr = temp_card_mem0_awaddr[`AXI_CARD_MEM_ADDR_WIDTH-1:0];

generate if(`AXI_CARD_MEM_ID_WIDTH > 1)
begin:high_cid_fields_driver
    assign m_axi_card_mem0_arid  [ `AXI_CARD_MEM_ID_WIDTH-1 : 1 ] = 'b0;
    assign m_axi_card_mem0_awid  [ `AXI_CARD_MEM_ID_WIDTH-1 : 1 ] = 'b0;
end
endgenerate

// if HBM
`else
   `ifdef HBM_AXI_IF_P0
assign m_axi_card_hbm_p0_araddr = temp_card_hbm_p0_araddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
assign m_axi_card_hbm_p0_awaddr = temp_card_hbm_p0_awaddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
   `endif
   `ifdef HBM_AXI_IF_P1
assign m_axi_card_hbm_p1_araddr = temp_card_hbm_p1_araddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
assign m_axi_card_hbm_p1_awaddr = temp_card_hbm_p1_awaddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
   `endif
   `ifdef HBM_AXI_IF_P2
assign m_axi_card_hbm_p2_araddr = temp_card_hbm_p2_araddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
assign m_axi_card_hbm_p2_awaddr = temp_card_hbm_p2_awaddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
   `endif
   `ifdef HBM_AXI_IF_P3
assign m_axi_card_hbm_p3_araddr = temp_card_hbm_p3_araddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
assign m_axi_card_hbm_p3_awaddr = temp_card_hbm_p3_awaddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
   `endif
   `ifdef HBM_AXI_IF_P4
assign m_axi_card_hbm_p4_araddr = temp_card_hbm_p4_araddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
assign m_axi_card_hbm_p4_awaddr = temp_card_hbm_p4_awaddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
   `endif
   `ifdef HBM_AXI_IF_P5
assign m_axi_card_hbm_p5_araddr = temp_card_hbm_p5_araddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
assign m_axi_card_hbm_p5_awaddr = temp_card_hbm_p5_awaddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
   `endif
   `ifdef HBM_AXI_IF_P6
assign m_axi_card_hbm_p6_araddr = temp_card_hbm_p6_araddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
assign m_axi_card_hbm_p6_awaddr = temp_card_hbm_p6_awaddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
   `endif
   `ifdef HBM_AXI_IF_P7
assign m_axi_card_hbm_p7_araddr = temp_card_hbm_p7_araddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
assign m_axi_card_hbm_p7_awaddr = temp_card_hbm_p7_awaddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
   `endif
   `ifdef HBM_AXI_IF_P8
assign m_axi_card_hbm_p8_araddr = temp_card_hbm_p8_araddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
assign m_axi_card_hbm_p8_awaddr = temp_card_hbm_p8_awaddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
   `endif
   `ifdef HBM_AXI_IF_P9
assign m_axi_card_hbm_p9_araddr = temp_card_hbm_p9_araddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
assign m_axi_card_hbm_p9_awaddr = temp_card_hbm_p9_awaddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
   `endif
   `ifdef HBM_AXI_IF_P10
assign m_axi_card_hbm_p10_araddr = temp_card_hbm_p10_araddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
assign m_axi_card_hbm_p10_awaddr = temp_card_hbm_p10_awaddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
   `endif
   `ifdef HBM_AXI_IF_P11
assign m_axi_card_hbm_p11_araddr = temp_card_hbm_p11_araddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
assign m_axi_card_hbm_p11_awaddr = temp_card_hbm_p11_awaddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
   `endif
   `ifdef HBM_AXI_IF_P12
assign m_axi_card_hbm_p12_araddr = temp_card_hbm_p12_araddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
assign m_axi_card_hbm_p12_awaddr = temp_card_hbm_p12_awaddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
   `endif
   `ifdef HBM_AXI_IF_P13
assign m_axi_card_hbm_p13_araddr = temp_card_hbm_p13_araddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
assign m_axi_card_hbm_p13_awaddr = temp_card_hbm_p13_awaddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
   `endif
   `ifdef HBM_AXI_IF_P14
assign m_axi_card_hbm_p14_araddr = temp_card_hbm_p14_araddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
assign m_axi_card_hbm_p14_awaddr = temp_card_hbm_p14_awaddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
   `endif
   `ifdef HBM_AXI_IF_P15
assign m_axi_card_hbm_p15_araddr = temp_card_hbm_p15_araddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
assign m_axi_card_hbm_p15_awaddr = temp_card_hbm_p15_awaddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
   `endif
   `ifdef HBM_AXI_IF_P16
assign m_axi_card_hbm_p16_araddr = temp_card_hbm_p16_araddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
assign m_axi_card_hbm_p16_awaddr = temp_card_hbm_p16_awaddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
   `endif
   `ifdef HBM_AXI_IF_P17
assign m_axi_card_hbm_p17_araddr = temp_card_hbm_p17_araddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
assign m_axi_card_hbm_p17_awaddr = temp_card_hbm_p17_awaddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
   `endif
   `ifdef HBM_AXI_IF_P18
assign m_axi_card_hbm_p18_araddr = temp_card_hbm_p18_araddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
assign m_axi_card_hbm_p18_awaddr = temp_card_hbm_p18_awaddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
   `endif
   `ifdef HBM_AXI_IF_P19
assign m_axi_card_hbm_p19_araddr = temp_card_hbm_p19_araddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
assign m_axi_card_hbm_p19_awaddr = temp_card_hbm_p19_awaddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
   `endif
   `ifdef HBM_AXI_IF_P20
assign m_axi_card_hbm_p20_araddr = temp_card_hbm_p20_araddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
assign m_axi_card_hbm_p20_awaddr = temp_card_hbm_p20_awaddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
   `endif
   `ifdef HBM_AXI_IF_P21
assign m_axi_card_hbm_p21_araddr = temp_card_hbm_p21_araddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
assign m_axi_card_hbm_p21_awaddr = temp_card_hbm_p21_awaddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
   `endif
   `ifdef HBM_AXI_IF_P22
assign m_axi_card_hbm_p22_araddr = temp_card_hbm_p22_araddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
assign m_axi_card_hbm_p22_awaddr = temp_card_hbm_p22_awaddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
   `endif
   `ifdef HBM_AXI_IF_P23
assign m_axi_card_hbm_p23_araddr = temp_card_hbm_p23_araddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
assign m_axi_card_hbm_p23_awaddr = temp_card_hbm_p23_awaddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
   `endif
   `ifdef HBM_AXI_IF_P24
assign m_axi_card_hbm_p24_araddr = temp_card_hbm_p24_araddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
assign m_axi_card_hbm_p24_awaddr = temp_card_hbm_p24_awaddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
   `endif
   `ifdef HBM_AXI_IF_P25
assign m_axi_card_hbm_p25_araddr = temp_card_hbm_p25_araddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
assign m_axi_card_hbm_p25_awaddr = temp_card_hbm_p25_awaddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
   `endif
   `ifdef HBM_AXI_IF_P26
assign m_axi_card_hbm_p26_araddr = temp_card_hbm_p26_araddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
assign m_axi_card_hbm_p26_awaddr = temp_card_hbm_p26_awaddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
   `endif
   `ifdef HBM_AXI_IF_P27
assign m_axi_card_hbm_p27_araddr = temp_card_hbm_p27_araddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
assign m_axi_card_hbm_p27_awaddr = temp_card_hbm_p27_awaddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
   `endif
   `ifdef HBM_AXI_IF_P28
assign m_axi_card_hbm_p28_araddr = temp_card_hbm_p28_araddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
assign m_axi_card_hbm_p28_awaddr = temp_card_hbm_p28_awaddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
   `endif
   `ifdef HBM_AXI_IF_P29
assign m_axi_card_hbm_p29_araddr = temp_card_hbm_p29_araddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
assign m_axi_card_hbm_p29_awaddr = temp_card_hbm_p29_awaddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
   `endif
   `ifdef HBM_AXI_IF_P30
assign m_axi_card_hbm_p30_araddr = temp_card_hbm_p30_araddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
assign m_axi_card_hbm_p30_awaddr = temp_card_hbm_p30_awaddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
   `endif
   `ifdef HBM_AXI_IF_P31
assign m_axi_card_hbm_p31_araddr = temp_card_hbm_p31_araddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
assign m_axi_card_hbm_p31_awaddr = temp_card_hbm_p31_awaddr[`AXI_CARD_HBM_ADDR_WIDTH-1:0];
   `endif

`endif
`endif
endmodule
