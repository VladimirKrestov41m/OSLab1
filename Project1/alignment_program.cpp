#include<windows.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define MAX_WIDTH 64

struct linesInfo {
	int count;//���-�� ������� � ������
	int size; //������ ������
	char** lines;//����� (������)
};

struct Buf_in_out {
	linesInfo* lines_in;//������� �����
	linesInfo* lines_out;//�������� �����
};

char** lines_in, ** lines_out;

CRITICAL_SECTION CriticalSectionReader, CriticalSectionWriter;

DWORD WINAPI ReadFile(LPVOID);
DWORD WINAPI Aligment_text(LPVOID);
DWORD WINAPI WriteFile(LPVOID);

void FreeResources(int size);

linesInfo* buf_in, * buf_out;
Buf_in_out* buf_in_out;

int main() {
	HANDLE threads[3];
	InitializeCriticalSection(&CriticalSectionReader);
	InitializeCriticalSection(&CriticalSectionWriter);
	int error = 0;
	int size = 8;
	int delta_size = 0;
	printf("Start!\n");
	printf("Buf size = %d\n", size);

	lines_in = (char**)malloc(size * sizeof(char*));
	lines_out = (char**)malloc(size * sizeof(char*));

	for (int i = 0; i < size; i++) {
		lines_in[i] = (char*)malloc(MAX_WIDTH * sizeof(char));
		memset(lines_in[i], '0', (MAX_WIDTH * sizeof(char)));
		lines_in[i][MAX_WIDTH - 1] = '\0';
		lines_out[i] = (char*)malloc(MAX_WIDTH * sizeof(char));
		memset(lines_out[i], '0', (MAX_WIDTH * sizeof(char)));
		lines_out[i][MAX_WIDTH - 1] = '\0';
	}

	buf_in = (linesInfo*)malloc(sizeof(linesInfo));
	buf_out = (linesInfo*)malloc(sizeof(linesInfo));
	buf_in->size = size;
	buf_out->size = size;
	buf_in->count = 0;
	buf_out->count = 0;
	buf_in->lines = lines_in;
	buf_out->lines = lines_out;
	buf_in_out = (Buf_in_out*)malloc(sizeof(Buf_in_out));
	buf_in_out->lines_in = buf_in;
	buf_in_out->lines_out = buf_out;

	threads[0] = CreateThread(NULL, 0, ReadFile, buf_in, 0, NULL);
	threads[1] = CreateThread(NULL, 0, Aligment_text, buf_in_out, 0, NULL);
	threads[2] = CreateThread(NULL, 0, WriteFile, buf_out, 0, NULL);

	if (threads[0] == NULL || threads[1] == NULL || threads[2] == NULL) {
		TerminateThread(threads[0], 0);
		TerminateThread(threads[1], 0);
		TerminateThread(threads[2], 0);
		FreeResources(size);
		printf("Procces was crashed!\n");
		system("pause");
		exit(0);
	}

	printf("Procceses are working.\n");

	WaitForMultipleObjects(3, threads, TRUE, INFINITE);

	for (int i = 0; i < 3; i++)
	{
		CloseHandle(threads[i]);
	}

	FreeResources(size);
	system("pause");
	return 0;
}

void FreeResources(int size)
{
	for (int i = 0; i < size; i++) {
		free(lines_in[i]);
		free(lines_out[i]);
	}
	free(lines_in);
	free(lines_out);
	free(buf_in);
	free(buf_out);
	free(buf_in_out);
}

DWORD WINAPI ReadFile(LPVOID param) {  //������ ������ �� ����� � ���������� ��� � ��������� �����
	linesInfo* buf_in = (linesInfo*)param;
	int buf_count = 0;//����� ����������� ������
	int next_line = 1;//����� ��������� ������
	int start_position = 0;
	bool ending = false;
	bool first_half = true, second_half = true;
	char symbol = ' ';
	int safeEndPosition = 0;
	bool isSafeWord = false;
	char partOfWord[32];
	FILE* f;
	f = fopen("text.txt", "r");
	fflush(f);
	rewind(f);

	if (f != NULL)
	{
		do
		{  //���� ����� ������, ���� �� ��������� ���� ����
			if (buf_in->count == 0)
			{
				next_line = 1;
			}

			if (buf_in->count < buf_in->size)
			{
				if (isSafeWord) //���� ���� ��������� �����, �� �������� ��� � ������ ������
				{
					isSafeWord = false;

					for (int i = 0; i < safeEndPosition; i++)
					{
						buf_in->lines[buf_in->count][i] = partOfWord[i];
					}

					start_position = safeEndPosition;
				}
				for (int j = start_position; j < MAX_WIDTH - 1; j++)
				{
					symbol = fgetc(f);

					if (symbol != EOF) //���� ������ �� ����� �����, �� ...
					{
						buf_in->lines[buf_in->count][j] = symbol;
					}
					else //���� ���� ��������, ��...
					{
						ending = true;
						break;
					}
				}
				if (!ending)
				{
					int y = MAX_WIDTH - 1;
					int position = 0; //������� �������. ������ �������� �������� �����

					do
					{
						symbol = buf_in->lines[buf_in->count][y];
						y--;
					} while (symbol != ' ');//������� ������ ���������� ������

					position = y;
					y = 0;
					symbol = ' ';

					if (buf_in->count == buf_in->size - 1) //���� ��� ��������� ������� ������, �� �������� ������� ����� � �����
					{
						for (int i = position + 2; i < MAX_WIDTH - 1; i++) //��������� ���������� ����� ����� ������� �� ����� �������
						{
							partOfWord[y] = buf_in->lines[buf_in->count][i];
							buf_in->lines[buf_in->count][i] = ' ';
							y++;
						}

						isSafeWord = true;
						safeEndPosition = y;
						EnterCriticalSection(&CriticalSectionReader);//����������� ������
						buf_in->count++;
						LeaveCriticalSection(&CriticalSectionReader);
					}
					else //�����...
					{
						for (int i = position + 2; i < MAX_WIDTH - 1; i++) //��������� ���������� ����� ����� ������� �� ����� �������
						{
							buf_in->lines[next_line][y] = buf_in->lines[buf_in->count][i];
							buf_in->lines[buf_in->count][i] = ' ';
							y++;
						}

						printf("%d: %s\n", strlen(buf_in->lines[buf_in->count]), buf_in->lines[buf_in->count]);
						start_position = y;
						next_line++;

						EnterCriticalSection(&CriticalSectionReader); //����������� ������
						buf_in->count++;
						LeaveCriticalSection(&CriticalSectionReader);
					}
				}
				else
				{
					EnterCriticalSection(&CriticalSectionReader); //����������� ������
					buf_in->count = -1;
					LeaveCriticalSection(&CriticalSectionReader);
					break;
				}
			}
			else
			{
				next_line = 1;
				Sleep(0);
			}
		} while (!ending);
	}

	fclose(f);
	printf("Read thread finished.---------------------------------------------------------------------------------\n");

	return 0;
}

DWORD WINAPI WriteFile(LPVOID param) //������ ������������� ������ � ����� ����
{
	linesInfo* buf_out = (linesInfo*)param, * delta_buf;
	printf("Thread 3: Start\n");
	char** delta_lines = (char**)malloc(buf_out->size * sizeof(char*));

	for (int i = 0; i < buf_out->size; i++) {
		delta_lines[i] = (char*)malloc(MAX_WIDTH * sizeof(char));
		memset(delta_lines[i], '0', (MAX_WIDTH * sizeof(char)));
		delta_lines[i][MAX_WIDTH - 1] = '\0';
	}

	delta_buf = (linesInfo*)malloc(sizeof(linesInfo));
	delta_buf->lines = delta_lines;
	delta_buf->count = 0;
	delta_buf->size = buf_out->size;
	FILE* f = fopen("completed_text.txt", "w");
	fflush(f);
	rewind(f);

	while (buf_out->count != -1) //���� ����� ����� ��� �� �������� ��������� ����� ������
	{
		if (buf_out->count == buf_out->size - 1)
		{
			EnterCriticalSection(&CriticalSectionWriter); //����������� ������
			buf_out->count = 0;

			for (int i = 0; i < buf_out->size; i++) //�������� ������ �� ������� ������ �� ��������� ����� ������� ������
			{
				strcpy(delta_buf->lines[i], buf_out->lines[i]);
			}

			LeaveCriticalSection(&CriticalSectionWriter);

			for (int i = 0; i < delta_buf->size; i++) //����� � ���� ������ � ������ �������� ������
			{
				fputs(delta_buf->lines[i], f);
				fputc('\n', f);
			}

			fflush(f);
		}
		else
		{
			Sleep(0);
		}
	}

	fclose(f);

	for (int i = 0; i < buf_out->size; i++) //��� �� ��� � ��������� ������ ???????????????????????????????????????
	{
		free(delta_lines[i]);
	}

	free(delta_lines);
	free(delta_buf);
	printf("Write thread finished.--------------------------------------------------------------------------------\n");

	return 0;
}

DWORD WINAPI Aligment_text(LPVOID param) //��������� ������ ���, ����� �� ��� �������� �� ������
{
	Buf_in_out* buf_in_out = (Buf_in_out*)param;
	int position = 0, step = 2; //������� � ���
	printf("Thread 2: Start\n");

	while (buf_in_out->lines_in->count != -1) //���� ����� ����� 1 �� �������� ������ ����
	{
		if (buf_in_out->lines_in->count == buf_in_out->lines_in->size //���� ������� ��������� ������ = ������� ������, � �������� ����� ������, �� ...
			&& buf_in_out->lines_out->count == 0)
		{
			EnterCriticalSection(&CriticalSectionReader);//����������� ������

			for (int i = 0; i < buf_in_out->lines_in->size; i++) //�������� ������ �� �������� ������ �� ��������
			{
				strcpy(buf_in_out->lines_out->lines[i], buf_in_out->lines_in->lines[i]);
			}

			buf_in_out->lines_in->count = 0;
			LeaveCriticalSection(&CriticalSectionReader);

			for (int i = 0; i < buf_in_out->lines_out->size; i++) //��� ������ ������, ���� � ����� ������ �� ����� �����-��
			{
				position = 0;
				step = 2;

				while (true) //������ ����� �������, ��������� ������� �� ���� ������
				{
					for (int j = 0; j < MAX_WIDTH - 1; j++) //������� ��� ����� ����� ������, ������� �� �� ������
					{
						if (buf_in_out->lines_out->lines[i][j] == '\n')
							buf_in_out->lines_out->lines[i][j] = ' ';
					}

					char d = buf_in_out->lines_out->lines[i][MAX_WIDTH - 2];

					if (buf_in_out->lines_out->lines[i][MAX_WIDTH - 2] == ' ') //���� ��������� ������ ������, ��...
					{
						for (int j = position; j < MAX_WIDTH - 2; j++) //������� ������
						{
							if (buf_in_out->lines_out->lines[i][j] == ' ') 
							{
								position = j;//���������� ������� �������
								break;
							}
						}

						for (int j = MAX_WIDTH - 2; j > position; j--) //� �����, ��������� ��� ������� �� ���� ������
						{
							buf_in_out->lines_out->lines[i][j] = buf_in_out->lines_out->lines[i][j - 1];
						}

						buf_in_out->lines_out->lines[i][position] = ' '; //�� ������� ������ ������
						position += 2; //��������� ������� ��� ���������� ����

						if (position > MAX_WIDTH - 2) 
						{
							step++;
							position = 0;
						}
					}
					else 
					{
						printf("Alignment line:    %s\n", buf_in_out->lines_out->lines[i]);
						step = 2;
						position = 0;
						break;
					}
				}

				EnterCriticalSection(&CriticalSectionWriter);//����������� ������
				buf_in_out->lines_out->count++;
				LeaveCriticalSection(&CriticalSectionWriter);
			}

			EnterCriticalSection(&CriticalSectionReader);//����������� ������
			buf_in_out->lines_out->count = buf_in_out->lines_out->size - 1;
			LeaveCriticalSection(&CriticalSectionReader);
		}
		else 
		{
			Sleep(0);
		}
	}

	EnterCriticalSection(&CriticalSectionReader); //����������� ������
	buf_in_out->lines_out->count = -1;
	LeaveCriticalSection(&CriticalSectionReader);
	printf("Aligment thread finished.-----------------------------------------------------------------------------\n");

	return 0;
}