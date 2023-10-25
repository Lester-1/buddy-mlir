//===- llama-main.cpp -----------------------------------------------------===//
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//===----------------------------------------------------------------------===//

#include <buddy/Core/Container.h>
#include <buddy/LLM/TextContainer.h>
#include <fstream>
#include <iostream>
#include <time.h>

using namespace buddy;
using namespace std;

extern "C" void _mlir_ciface_forward(MemRef<float, 3> *, MemRef<float, 1> *,
                                     MemRef<size_t, 2> *);

int main() {
  // Guide the user to enter the vocab path
  string vocabDir = "../../tests/Interface/core/vocab_llama.txt";
  // cout<<"please input vocab file path"<<endl;
  // getline(cin, vocabDir);

  // Initialize the container
  string pureStr;
  cout << "Please enter what you want to say to me" << endl;
  getline(cin, pureStr);
  clock_t buddyTokenizeStart = clock();
  Text<size_t, 2> pureStrContainer(pureStr);
  pureStrContainer.tokenizeLlama(vocabDir, 80);
  clock_t buddyTokenizeEnd = clock();
  double buddyTokenizeTime =
      (double)(buddyTokenizeEnd - buddyTokenizeStart) / CLOCKS_PER_SEC;
  // Print the tokenized result
  cout << "Get User input:" << pureStrContainer.revertLlama(pureStrContainer)
       << endl;
  cout << "[Buddy] Tokenize input time: " << buddyTokenizeTime * 1000 << "ms"
       << endl;

  // Read the params
  clock_t buddyReadStart = clock();
  cout << "Please input params file directory" << endl;
  string paramsPath;
  getline(cin, paramsPath);
  MemRef<float, 1> arg0({intptr_t(6755192832)});
  ifstream in0(paramsPath + "/arg0.data", ios::in | ios::binary);
  if (!in0.is_open()) {
    throw std::runtime_error("Failed to open param file!");
  }
  in0.read((char *)(arg0.getData()), sizeof(float) * (arg0.getSize()));
  in0.close();
  clock_t buddyReadEnd = clock();
  double buddyReadTime =
      (double)(buddyReadEnd - buddyReadStart) / CLOCKS_PER_SEC;
  cout << "Read params finish" << endl;
  cout << "[Buddy] Read params time: " << buddyReadTime << "s" << endl;

  // Run the model
  MemRef<float, 3> result({1, 80, 32000});
  int generateLen = 80 - pureStrContainer.getTokenCnt();
  cout << "-----------------------start generate-----------------------"
       << endl;
  clock_t buddyStart = clock();
  for (int i = 0; i < generateLen; i++) {
    cout << "Iteration" << i << ": ";
    buddyReadStart = clock();
    // Perform calculations in memref generated by user input.
    _mlir_ciface_forward(&result, &arg0, &pureStrContainer);
    int tokenIndex = pureStrContainer.getTokenCnt() - 1;
    int index = 0;
    int maxEle = result.getData()[tokenIndex * 32000];
    // Calculate the probability of occurrence of each token.
    for (int j = index + 1; j < 32000; j++) {
      if (result.getData()[tokenIndex * 32000 + j] > maxEle) {
        maxEle = result.getData()[tokenIndex * 32000 + j];
        index = j;
      }
    }
    pureStrContainer.getData()[pureStrContainer.getTokenCnt()] = index;
    // If the model generate 2(sep marker), interrupt generation immediately.
    if (index == 2) {
      break;
    }
    buddyReadEnd = clock();
    buddyReadTime = (double)(buddyReadEnd - buddyReadStart) / CLOCKS_PER_SEC;
    cout << pureStrContainer.getStr(index) << endl;
    cout << "[Buddy] Llama iteration " << i << " time: " << buddyReadTime << "s"
         << endl;
    pureStrContainer.setTokenCnt(pureStrContainer.getTokenCnt() + 1);
  }
  cout << "------------------------------------------------------------"
       << endl;
  // Statistics running time
  clock_t buddyEnd = clock();
  buddyReadTime = (double)(buddyEnd - buddyStart) / CLOCKS_PER_SEC;
  // Print the result
  cout << "[Buddy] Result: " << pureStrContainer.revertLlama(pureStrContainer)
       << endl;
  cout << "[Buddy] Llama exection time: " << buddyReadTime << "s" << endl;
  return 0;
}
