
import React, { useState } from 'react';
import { PillarData } from '../types';
import CodeBlock from './CodeBlock';
import { ChevronDownIcon, ChevronUpIcon } from '@heroicons/react/24/solid';

interface PillarCardProps {
  pillar: PillarData;
}

const PillarCard: React.FC<PillarCardProps> = ({ pillar }) => {
  const [isExpanded, setIsExpanded] = useState(false);

  return (
    <div className="bg-white rounded-lg shadow-xl overflow-hidden mb-8 transition-all duration-300 ease-in-out">
      <button
        onClick={() => setIsExpanded(!isExpanded)}
        className="w-full text-left p-6 bg-primary hover:bg-blue-700 text-white flex justify-between items-center transition-colors duration-200"
      >
        <div>
          <h3 className="text-2xl font-bold">{pillar.title}</h3>
          {pillar.alias && <p className="text-blue-200 text-lg italic">{pillar.alias}</p>}
        </div>
        {isExpanded ? <ChevronUpIcon className="w-6 h-6" /> : <ChevronDownIcon className="w-6 h-6" />}
      </button>

      {isExpanded && (
        <div className="p-6 space-y-6">
          <div className="text-lg text-text-secondary leading-relaxed prose prose-lg max-w-none">
            {typeof pillar.description === 'string' ? <p>{pillar.description}</p> : pillar.description}
          </div>

          {pillar.architecture && (
            <div>
              <h4 className="text-xl font-semibold text-primary mb-2">Component Architecture</h4>
              <div className="text-text-secondary prose prose-lg max-w-none">{typeof pillar.architecture === 'string' ? <p>{pillar.architecture}</p> : pillar.architecture}</div>
            </div>
          )}

          {pillar.dataFlow && (
            <div>
              <h4 className="text-xl font-semibold text-primary mb-2">Data Flow</h4>
              <div className="text-text-secondary prose prose-lg max-w-none">{typeof pillar.dataFlow === 'string' ? <p>{pillar.dataFlow}</p> : pillar.dataFlow}</div>
            </div>
          )}

          {pillar.coreStructures && (
            <div>
              <h4 className="text-xl font-semibold text-primary mb-2">Core Data Structures/Functions</h4>
              <div className="text-text-secondary prose prose-lg max-w-none">{typeof pillar.coreStructures === 'string' ? <p>{pillar.coreStructures}</p> : pillar.coreStructures}</div>
            </div>
          )}

          {pillar.subSections?.map((section, index) => (
            <div key={index}>
              <h4 className="text-xl font-semibold text-primary mb-2">{section.title}</h4>
              <div className="text-text-secondary prose prose-lg max-w-none">
                {typeof section.content === 'string' ? <p>{section.content}</p> : section.content}
              </div>
            </div>
          ))}

          {pillar.codeExamples?.map((example, index) => (
            <div key={index} className="mt-4">
              <h5 className="text-lg font-semibold text-gray-700 mb-2">{example.title}</h5>
              <CodeBlock code={example.code} language={example.language} />
            </div>
          ))}
        </div>
      )}
    </div>
  );
};

export default PillarCard;
