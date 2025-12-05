# msptiActivityRegisterCallbacks<a name="ZH-CN_TOPIC_0000002014413733"></a>

## AI处理器支持情况<a name="section8178181118225"></a>

>![](public_sys-resources/icon-note.gif) **说明：** 
>AI处理器与昇腾产品的对应关系，请参见《[昇腾产品形态说明](https://www.hiascend.com/document/detail/zh/AscendFAQ/ProduTech/productform/hardwaredesc_0001.html)》。

<a name="table38301303189"></a>
<table><thead align="left"><tr id="row20831180131817"><th class="cellrowborder" valign="top" width="57.99999999999999%" id="mcps1.1.3.1.1"><p id="p1883113061818"><a name="p1883113061818"></a><a name="p1883113061818"></a><span id="ph20833205312295"><a name="ph20833205312295"></a><a name="ph20833205312295"></a>AI处理器类型</span></p>
</th>
<th class="cellrowborder" align="center" valign="top" width="42%" id="mcps1.1.3.1.2"><p id="p783113012187"><a name="p783113012187"></a><a name="p783113012187"></a>是否支持</p>
</th>
</tr>
</thead>
<tbody><tr id="row220181016240"><td class="cellrowborder" valign="top" width="57.99999999999999%" headers="mcps1.1.3.1.1 "><p id="p48327011813"><a name="p48327011813"></a><a name="p48327011813"></a><span id="ph583230201815"><a name="ph583230201815"></a><a name="ph583230201815"></a><term id="zh-cn_topic_0000001312391781_term1253731311225"><a name="zh-cn_topic_0000001312391781_term1253731311225"></a><a name="zh-cn_topic_0000001312391781_term1253731311225"></a>Ascend 910C</term></span></p>
</td>
<td class="cellrowborder" align="center" valign="top" width="42%" headers="mcps1.1.3.1.2 "><p id="p7948163910184"><a name="p7948163910184"></a><a name="p7948163910184"></a>√</p>
</td>
</tr>
<tr id="row173226882415"><td class="cellrowborder" valign="top" width="57.99999999999999%" headers="mcps1.1.3.1.1 "><p id="p14832120181815"><a name="p14832120181815"></a><a name="p14832120181815"></a><span id="ph1483216010188"><a name="ph1483216010188"></a><a name="ph1483216010188"></a><term id="zh-cn_topic_0000001312391781_term11962195213215"><a name="zh-cn_topic_0000001312391781_term11962195213215"></a><a name="zh-cn_topic_0000001312391781_term11962195213215"></a>Ascend 910B</term></span></p>
</td>
<td class="cellrowborder" align="center" valign="top" width="42%" headers="mcps1.1.3.1.2 "><p id="p19948143911820"><a name="p19948143911820"></a><a name="p19948143911820"></a>√</p>
</td>
</tr>
<tr id="row103361763242"><td class="cellrowborder" valign="top" width="57.99999999999999%" headers="mcps1.1.3.1.1 "><p id="p1983180181813"><a name="p1983180181813"></a><a name="p1983180181813"></a><span id="ph783112021813"><a name="ph783112021813"></a><a name="ph783112021813"></a><term id="zh-cn_topic_0000001312391781_term354143892110"><a name="zh-cn_topic_0000001312391781_term354143892110"></a><a name="zh-cn_topic_0000001312391781_term354143892110"></a>Ascend 310B</term></span></p>
</td>
<td class="cellrowborder" align="center" valign="top" width="42%" headers="mcps1.1.3.1.2 "><p id="p1695483941817"><a name="p1695483941817"></a><a name="p1695483941817"></a>√</p>
</td>
</tr>
<tr id="row18403312418"><td class="cellrowborder" valign="top" width="57.99999999999999%" headers="mcps1.1.3.1.1 "><p id="p78319051815"><a name="p78319051815"></a><a name="p78319051815"></a><span id="ph1383116081815"><a name="ph1383116081815"></a><a name="ph1383116081815"></a><term id="zh-cn_topic_0000001312391781_term4363218112215"><a name="zh-cn_topic_0000001312391781_term4363218112215"></a><a name="zh-cn_topic_0000001312391781_term4363218112215"></a>Ascend 310P</term></span></p>
</td>
<td class="cellrowborder" align="center" valign="top" width="42%" headers="mcps1.1.3.1.2 "><p id="p1695443971810"><a name="p1695443971810"></a><a name="p1695443971810"></a>x</p>
</td>
</tr>
<tr id="row17253142120252"><td class="cellrowborder" valign="top" width="57.99999999999999%" headers="mcps1.1.3.1.1 "><p id="p38313021813"><a name="p38313021813"></a><a name="p38313021813"></a><span id="ph58317041819"><a name="ph58317041819"></a><a name="ph58317041819"></a><term id="zh-cn_topic_0000001312391781_term71949488213"><a name="zh-cn_topic_0000001312391781_term71949488213"></a><a name="zh-cn_topic_0000001312391781_term71949488213"></a>Ascend 910</term></span></p>
</td>
<td class="cellrowborder" align="center" valign="top" width="42%" headers="mcps1.1.3.1.2 "><p id="p395243920186"><a name="p395243920186"></a><a name="p395243920186"></a>x</p>
</td>
</tr>
</tbody>
</table>

## 功能说明<a name="section20806203412478"></a>

向MSPTI注册回调函数，用于Activity Buffer处理。当Activity Buffer空间不足时会调用funcBufferRequested函数申请内存；Activity Buffer空间占满时调用funcBufferCompleted函数通知用户消费Activity数据，并释放Activity Buffer空间。

## 函数原型<a name="section1121883194711"></a>

```cpp
msptiResult msptiActivityRegisterCallbacks(msptiBuffersCallbackRequestFunc funcBufferRequested, msptiBuffersCallbackCompleteFunc funcBufferCompleted)
```

## 参数说明<a name="section11506138144714"></a>

**表 1**  参数说明

<a name="table827101275518"></a>
<table><thead align="left"><tr id="row429121265517"><th class="cellrowborder" valign="top" width="28.65286528652865%" id="mcps1.2.4.1.1"><p id="p1329121214558"><a name="p1329121214558"></a><a name="p1329121214558"></a>参数名</p>
</th>
<th class="cellrowborder" valign="top" width="13.661366136613662%" id="mcps1.2.4.1.2"><p id="p10230141454318"><a name="p10230141454318"></a><a name="p10230141454318"></a>输入/输出</p>
</th>
<th class="cellrowborder" valign="top" width="57.68576857685769%" id="mcps1.2.4.1.3"><p id="p83121275519"><a name="p83121275519"></a><a name="p83121275519"></a>说明</p>
</th>
</tr>
</thead>
<tbody><tr id="row1131131265511"><td class="cellrowborder" valign="top" width="28.65286528652865%" headers="mcps1.2.4.1.1 "><p id="p7669321185110"><a name="p7669321185110"></a><a name="p7669321185110"></a>funcBufferRequested</p>
</td>
<td class="cellrowborder" valign="top" width="13.661366136613662%" headers="mcps1.2.4.1.2 "><p id="p168851111619"><a name="p168851111619"></a><a name="p168851111619"></a>输入</p>
</td>
<td class="cellrowborder" valign="top" width="57.68576857685769%" headers="mcps1.2.4.1.3 "><p id="p131994242276"><a name="p131994242276"></a><a name="p131994242276"></a>Activity Buffer内存申请函数。</p>
</td>
</tr>
<tr id="row18118485118"><td class="cellrowborder" valign="top" width="28.65286528652865%" headers="mcps1.2.4.1.1 "><p id="p17567995224"><a name="p17567995224"></a><a name="p17567995224"></a>funcBufferCompleted</p>
</td>
<td class="cellrowborder" valign="top" width="13.661366136613662%" headers="mcps1.2.4.1.2 "><p id="p18311931172217"><a name="p18311931172217"></a><a name="p18311931172217"></a>输入</p>
</td>
<td class="cellrowborder" valign="top" width="57.68576857685769%" headers="mcps1.2.4.1.3 "><p id="p61315168385"><a name="p61315168385"></a><a name="p61315168385"></a>Activity Buffer数据消费和内存释放函数。</p>
</td>
</tr>
</tbody>
</table>

## 返回值说明<a name="section16621124213476"></a>

返回MSPTI\_SUCCESS表示成功，funcBufferRequested或funcBufferCompleted为NULL时返回MSPTI\_ERROR\_INVALID\_PARAMETER，表示失败。

