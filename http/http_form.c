#include "http.h"
#include "http_basic.h"
#include "http_api.h"


http_file_t * http_get_file(http_t* info,int num)
{

        http_file_ctrl_t* file=http_get_file_t(info);
        if( num >=file->num){
                return NULL;
        }

        return  &file->file[num];
}


http_file_t* http_get_empty_file(http_t* info)
{
        http_file_ctrl_t* ctl=http_get_file_t(info);
        http_file_t* file=NULL;
        if( ctl->num>= 8){
                return NULL;
        }
        file=&ctl->file[ctl->num++];
        return file;
}

int
http_get_file_cnt(http_t* info)
{
        http_file_ctrl_t *file=http_get_file_t(info);
        int num=file->num;
	return num;

}

int http_set_file(char* name,int name_len,char* value,int value_len,http_t* info,http_file_t* item)
{
        if( item == NULL){
                return 0;
        }

        x_str_t* fname=&item->name;
        if( http_get_file_cnt(info)>= 8){
                return -1;
        }
        item->raw_file_len=value_len;
        int ret=http_set_str(info,fname,name,name_len);
	return ret;

}
int http_append_file(char* value,int value_len,http_t* info,http_file_t* item)
{
        if( item == NULL){
                return 0;
        }
        if( http_get_file_cnt(info)>= 8){
                return -1;
        }
        x_str_t* fvalue=&item->value;
        if( fvalue->len+value_len >HTTP_FILE_MAX_LEN){
                item->complete_flag=1;
                return 0;
        }

        return http_append_str(info,fvalue,value,value_len);
}

int 
http_wait_label_str(char* buf,int buf_len,char* data,int data_len, 
	      http_t* info,char* str,int str_len)
{
        int len=0;
        local_t* local=http_get_local_t(info);

	int ret=-1;
	len=strlcpy2(local->cache_buf+local->cache_buf_len,
                        sizeof(local->cache_buf)-local->cache_buf_len,
                        data,data_len);
        local->cache_buf_len=local->cache_buf_len+len;

	if( local->cache_buf_len == str_len-local->boundary_cnt){
		if( memcmp(local->cache_buf,str+local->boundary_cnt,str_len-local->boundary_cnt) == 0){
			ret=strlcpy2(buf,buf_len,local->cache_buf,local->cache_buf_len);
		}else{
			ret=-1;
		}
        	local->cache_buf_len=0;
		local->myform_label_state=LABEL_STATE_NULL;
	}
	return ret;

}
int http_wait_line_str(char* buf,int buf_len,char* data,int data_len, 
	      http_t* info,char* str,int str_len)
{
	int ret=-1;
	int len=0;
        local_t* local=http_get_local_t(info);
        len=strlcpy2(local->cache_buf+local->cache_buf_len,
                        sizeof(local->cache_buf)-local->cache_buf_len,
                        data,data_len);
        local->cache_buf_len=local->cache_buf_len+len;
	switch(*data){
		case '\r':
			if( local->form_header_state == 0){	
				local->form_header_state=1;
			}else if( local->form_header_state ==2){
				local->form_header_state=3;
			}else{
			}
			break;
		case '\n':
			if( local->form_header_state == 1){	
				local->form_header_state=2;
			}else if( local->form_header_state ==3){
				local->form_header_state=4;
				ret=local->cache_buf_len;	
        			local->cache_buf_len=0;
				local->form_header_state=0;
			}else{
			}
			break;
	
		default:
			local->form_header_state=0;
			ret=-1;
			break;
			
	}
	return ret;

}
int 
http_get_form_lable_value(char* data,int len,char* label,int label_len,
		char* name,int* name_len,int max_len)
{
	char* begin=NULL;
	char* end=NULL;
	begin=strlstr(data,len,label,label_len);
	if( begin == NULL){
		return -1;
	}
	begin=begin+label_len;
	end=strlstr(begin,data+len-begin,"\"",1);
	if( end== NULL){
		return -1 ;
	}
	*name_len=strlcpy2(name,max_len,begin,end-begin);
	return 0;

}
int 
http_get_form_lable_value2(char* data,int len,char* label,int label_len,
		char** name,int* name_len,int max_len)
{
	char* begin=NULL;
	char* end=NULL;
	begin=strlstr(data,len,label,label_len);
	if( begin == NULL){
		return -1;
	}
	begin=begin+label_len;
	end=strlstr(begin,data+len-begin,"\"",1);
	if( end== NULL){
		return -1;
	}
	*name=begin;
	*name_len=end-begin;
	return 0;

}

int http_parse_form(char* data,int len,http_t* info)
{
	char* mydata=data;
	int mylen=len;
	int ret=0;
	int ret_len=0;
	char buf[256]={0};
	char label[256]={0};
	char file_name[512];
	int file_name_len=0;
	char name[512];
	int name_len=0;
	int label_len=0;
	char* name_label="name=\"";
	int name_label_len=strlen("name=\"");
	char* file_label="filename=\"";
	int file_label_len=strlen("filename=\"");

	local_t* local=http_get_local_t(info);	
	http_str_t* pro=http_get_str_t(info);	

	snprintf(label,sizeof(label),"--%s",pro->boundary.p);
	label_len=strlen(label);

	http_file_ctrl_t* file=http_get_file_t(info);
	http_arg_ctrl_t* arg=http_get_arg_t(info);
	int myret=0;
	while(mylen>0){
		if( local->form_status == HTTP_FORM_BEGIN){
			switch(*mydata){
				case '-':
					local->myform_label_num++;
					if( local->myform_label_num==local->boundary_cnt){
						local->myform_label_state=LABEL_STATE_BEGIN;
					}
					break;
				default:
					if( local->myform_label_state==LABEL_STATE_BEGIN){
						local->myform_label_state=LABEL_STATE_RUN;
					}else {
					}
					if( local->myform_label_state!=LABEL_STATE_RUN){
						local->myform_label_state=LABEL_STATE_NULL;
						local->myform_label_num=0;
					}
					local->myform_label_num=0;

					break;
			}
			if( local->myform_label_state == LABEL_STATE_RUN){
				ret=http_wait_label_str(buf,sizeof(buf),mydata,1,info,label,label_len);
			}else{
                		ret=-1;
			}
	
			mydata++;
			mylen--;
			if( ret ==-1){
			}else{
				local->form_status =HTTP_FORM_HEADER;
				local->form_flag =HTTP_FORM_ARG;
			}
			continue;
		}else if( local->form_status == HTTP_FORM_HEADER){
			ret_len=http_wait_line_str(buf,sizeof(buf),mydata,1,info,"\r\n\r\n",4);
			mydata++;
			mylen--;
			if( ret_len ==-1){
			}else{
				local->form_status =HTTP_FORM_BODY;
				ret=http_get_form_lable_value(local->cache_buf,ret_len,
						file_label,file_label_len,file_name,&file_name_len,sizeof(file_name));
				if( ret == 0){
					if( file->cur != NULL){
						//file->cur->value.p[file->cur->value.len-2]='\0';
					}
					if( file_name_len>0){

						local->null_file=0;
						file->cur=http_get_empty_file( info);
						if ( file ->cur == NULL){
							//	return MEM_ERR;
						}
				 		if ( file->cur != NULL ){
							file->cur->complete_flag=0;
						}
						myret=http_set_file(file_name,file_name_len,"\0",0,info,file->cur);
						local->form_flag=HTTP_FORM_FILE;
					}else{
						local->null_file=1;
					}
				}
				ret=http_get_form_lable_value(local->cache_buf,ret_len,
						name_label,name_label_len,name,&name_len,sizeof(file_name));
				if( local->form_flag == HTTP_FORM_ARG){
					arg->cur=http_get_empty_arg( info);
					if( arg->cur == NULL){
						return -1;
					}

					myret=http_append_arg(name,name_len,info,arg->cur);
					arg->cur->arg_flag=0;
					myret=http_append_arg("\0",1,info,arg->cur);
					arg->cur->arg_flag=1;
				}
			}
			continue;
		}else if( local->form_status == HTTP_FORM_BODY){
			switch(*mydata){
				case '-':
					local->myform_label_num++;
					if( local->myform_label_num==local->boundary_cnt){
						local->myform_label_state=LABEL_STATE_BEGIN;
					}
					break;
				default:
					if( local->myform_label_state==LABEL_STATE_BEGIN){
						local->myform_label_state=LABEL_STATE_RUN;
						//local->myform_label_num_fix=local->myform_label_num-local->boundary_cnt;
						//if( local->myform_label_num_fix<0){
						//	local->myform_label_num_fix=0;
						//}
					}else {
					}
					if( local->myform_label_state!=LABEL_STATE_RUN){
						local->myform_label_state=LABEL_STATE_NULL;
					}
					local->myform_label_num=0;

					break;
			}
	
			if( local->myform_label_state == LABEL_STATE_RUN){
				ret=http_wait_label_str(buf,sizeof(buf),mydata,1,info,label,label_len);
			}else{
                		ret=-1;
			}
	


			if( ret ==-1){
				if( local->form_flag ==HTTP_FORM_FILE){

					if( file->cur != NULL && local->null_file==0){
						http_file_t* item=file->cur;
						item->raw_file_len=item->raw_file_len+1;
						if( item->value.len+1 >HTTP_FILE_MAX_LEN){
							item->complete_flag=1;
						
						}else{

							ret=http_append_file(mydata,1,info,file->cur);
						}
					}
				}else{
					ret=http_append_arg(mydata,1,info,arg->cur);
				}
			}else{
				if( local->form_flag ==HTTP_FORM_FILE){
				 	if ( file->cur != NULL &&file->cur->value.p!= NULL && file->cur->value.len>0&& local->null_file==0 ){
						file->cur->value.len=file->cur->value.len-label_len-1;
						if( file->cur->value.len<0){
							file->cur->value.len=0;
						}
						file->cur->raw_file_len=file->cur->raw_file_len-label_len-1;
						if( file->cur->raw_file_len <0){
							file->cur->raw_file_len=0;
						}
						if( file->cur->value.len >0){
							file->cur->value.p[file->cur->value.len]='\0';
						}
						file->cur->complete_flag=1;
					}
				}else{
					if ( arg->cur != NULL && arg->cur->value.p!= NULL  && arg->cur->value.len>0){
						arg->cur->value.len=arg->cur->value.len-label_len-1;
						if( arg->cur->value.len<0){
							arg->cur->value.len=0;
						}
						if( arg->cur->value.len >0){
							arg->cur->value.p[arg->cur->value.len]='\0';
						}
					}
				}
				local->form_status =HTTP_FORM_HEADER;
				local->form_flag=HTTP_FORM_ARG;
			}
			mydata++;
			mylen--;
			continue;
		}

	}
	if( http_get_logic(info,HTTP_LOGIC_STATE) == HTTP_STATE_NONE){
		if( local->form_flag ==HTTP_FORM_FILE ){
		 	if ( file->cur != NULL &&file->cur->value.p!= NULL && file->cur->value.len>0 && local->null_file ==0 ){
				file->cur->value.len=file->cur->value.len-label_len-2;
				if( file->cur->value.len<0){
					file->cur->value.len=0;
				}
				file->cur->raw_file_len=file->cur->raw_file_len-label_len-2;
				if( file->cur->raw_file_len <0){
					file->cur->raw_file_len=0;
				}
				if( file->cur->value.len >0){
					file->cur->value.p[file->cur->value.len]='\0';
				}
				file->cur->complete_flag=1;
			}
		}else{
			if ( arg->cur != NULL && arg->cur->value.p!= NULL &&  arg->cur->value.p[0] !='\0' && arg->cur->value.len>0){
				arg->cur->value.len=arg->cur->value.len-label_len-2;
				if( arg->cur->value.len<0){
					arg->cur->value.len=0;
				}
				if( arg->cur->value.len >0){
					arg->cur->value.p[arg->cur->value.len]='\0';
				}
			}
		}
				
	}
	return 0;
}


