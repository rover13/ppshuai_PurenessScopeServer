##�˹������Ŀ��

�˹�����PSS�Ŀ�ƽ̨�ͻ��˹�����֮һ��������߿��Լ��ָ����̨��PSS���������������������
* ��Ծ������ActiveClient����ǰPSS�������Ѿ����ӵĿͻ���������
* ���ӳ�����PoolClient����ǰPSS�����е����ӳصĴ�С��
* ���������������MaxHandlerCount����ǰPSS����������������
* FlowIn����ǰPSS������յĿͻ���������λ�ܺͣ�1����ˢ��һ�Σ�
* FlowOut ��ǰPSS�����͵Ŀͻ���������λ�ܺͣ�1����ˢ��һ�Σ�
   
ͨ�����������ļ����Դ������Ŀ��(Main.xml)��
�����������Ľ������䡣
���磺  
* * * 
<code>
\<Mail MailID="0" fromMailAddr="local@163.com" toMailAddr="aaaa@163.com" MailPass="123456" MailUrl="smtp.163.com" MailPort="25" />
</code>
* * * 

������ͨ������ָ����XML���ã�ʵ��ͬʱ��ض�̨PSS��Ŀ�ꡣ
* * *
<code>
\<TCPServerIP name="PSS1" ip="127.0.0.1" port="10010" ipType="IPV4" key="freeeyes" />  
\<TCPServerIP name="PSS1" ip="127.0.0.2" port="10010" ipType="IPV4" key="freeeyes1" />
</code>
* * *

������ʵ���ˣ���ָ����PSS��������������м�أ�������ؽ������ΪXMLҳ�档���XMLҳ�����ͨ��http����ֱ��չʾ�ɽ��档  
����XMLλ��ΪMonitorLog/������name/name_date.xml